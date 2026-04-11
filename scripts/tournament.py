import argparse
import sys
import json
import re
import subprocess

from pathlib import Path
from math import ceil, log2
import random

class Tournament:
    @staticmethod
    def _new_match(match_id, round_index, slot_a, slot_b):
        return {
            "id": match_id,
            "round": round_index,
            "slot_a": slot_a,
            "slot_b": slot_b,
            "team_a": None,
            "team_b": None,
            "winner": None,
            "loser": None,
            "played": False,
        }

    @classmethod
    def from_config(cls, config_file):
        with open(config_file) as config:
            data = json.load(config)

        name = str(data["name"]).strip()
        participants = [str(t).strip() for t in data["participants"] if str(t).strip()]
        

        levels = data.get("levels", ["."])

        if not name:
            sys.exit("Error: Tournament name cannot be empty")
        if len(participants) < 2:
            sys.exit("Error: At least two participants are required")

        return cls(name, participants, levels)

    def __init__(self, name, participants, levels=None):
        self.name = name
        self.participants = participants
        self.levels = levels or ["."]
        self.bracket = []
        self.results = []
        self.finished = False

        self._build_tree()
        
        if len(self.levels) < len(self.bracket):
            sys.exit(f"Error: Faltan conjuntos de niveles en el archivo de configuración. Se necesitan {len(self.bracket)} conjuntos para {len(self.participants)} participantes, pero se proporcionaron solo {len(self.levels)}.")

    def _ascii_tree(self):
        if not self.bracket: return ""
        main_bracket = []
        for rnd in self.bracket:
            main_rnd = [m for m in rnd if m["id"] != "THIRD_PLACE"]
            if main_rnd: main_bracket.append(main_rnd)
        if not main_bracket: return ""

        N = len(main_bracket[0]) * 2
        rows = 2 * N - 1
        cell_w = 16
        grid = [[" " for _ in range(len(main_bracket) * (cell_w + 4) + cell_w)] for _ in range(rows)]
        
        def put_str(r, c, s):
            for i, char in enumerate(s):
                if c + i < len(grid[0]) and r < rows: grid[r][c + i] = char

        def get_name(match, t_key, s_key):
            if match.get(t_key): return str(match[t_key])
            slot = match.get(s_key)
            if isinstance(slot, str): return slot
            if isinstance(slot, dict): return "TBD"
            if slot is None: return "BYE"
            return "TBD"

        match_rows = {}
        for r_idx, rnd in enumerate(main_bracket):
            col = r_idx * (cell_w + 4)
            for m_idx, match in enumerate(rnd):
                if r_idx == 0:
                    row_a = 4 * m_idx
                    row_b = 4 * m_idx + 2
                    row_out = 4 * m_idx + 1
                else:
                    src_a = match["slot_a"]["match_id"]
                    src_b = match["slot_b"]["match_id"]
                    if src_a in match_rows and src_b in match_rows:
                        row_a = match_rows[src_a]
                        row_b = match_rows[src_b]
                        row_out = (row_a + row_b) // 2
                    else:
                        continue
                    
                match_rows[match["id"]] = row_out
                t_a = get_name(match, "team_a", "slot_a")
                t_b = get_name(match, "team_b", "slot_b")
                
                win_a = (match["winner"] == t_a and match["played"]) 
                win_b = (match["winner"] == t_b and match["played"])
                
                lbl_a = f"{t_a}{'*' if win_a else ''}"[:cell_w-1]
                lbl_b = f"{t_b}{'*' if win_b else ''}"[:cell_w-1]
                
                put_str(row_a, col, lbl_a)
                put_str(row_b, col, lbl_b)
                
                for c in range(col + len(lbl_a), col + cell_w): grid[row_a][c] = "─"
                grid[row_a][col + cell_w] = "┐"
                
                for c in range(col + len(lbl_b), col + cell_w): grid[row_b][c] = "─"
                grid[row_b][col + cell_w] = "┘"
                
                for r in range(row_a + 1, row_b): grid[r][col + cell_w] = "│"
                    
                grid[row_out][col + cell_w] = "├"
                for c in range(col + cell_w + 1, col + cell_w + 4): grid[row_out][c] = "─"

        final_match = main_bracket[-1][0]
        if final_match["id"] in match_rows:
            final_row = match_rows[final_match["id"]]
            final_col = len(main_bracket) * (cell_w + 4)
            winner = final_match["winner"]
            if winner: put_str(final_row, final_col, f"👑 {winner}")
            else: put_str(final_row, final_col, "TBD")
            
        lines = ["".join(row).rstrip() for row in grid]
        while lines and not lines[-1]: lines.pop()
            
        third_place = None
        if self.bracket and any(m["id"] == "THIRD_PLACE" for m in self.bracket[-1]):
            for m in self.bracket[-1]:
                if m["id"] == "THIRD_PLACE":
                    third_place = m; break
            
        if third_place:
            lines.append("")
            lines.append("-" * 30)
            t_a = get_name(third_place, "team_a", "slot_a")
            t_b = get_name(third_place, "team_b", "slot_b")
            w = third_place.get("winner") or "TBD"
            lines.append(f"🥉 TERCER PUESTO: {t_a} vs {t_b} -> Ganador: {w}")
            
        return "\n".join(lines)

    def __str__(self):
        total = sum(len(rounds) for rounds in self.bracket)
        played = sum(1 for match in self._all_matches() if match["played"])
        header = f"Tournament({self.name}) {played}/{total} matches"
        try:
            tree = self._ascii_tree()
            if tree: return f"\n{header}\n\n{tree}\n"
        except Exception:
            pass
        return header

    def _build_tree(self):
        teams = self.participants[:]
        random.shuffle(teams)
        size = 1 << ceil(log2(len(teams)))
        free = size - len(teams)
        teams.extend([None] * free)

        # Generar orden de cruces estándar para repartir los BYEs
        seeds = [0]
        while len(seeds) < size:
            curr_len = len(seeds)
            next_seeds = []
            for s in seeds:
                next_seeds.append(s)
                next_seeds.append(curr_len * 2 - 1 - s)
            seeds = next_seeds
            
        ordered_teams = [teams[i] for i in seeds]

        first_round = []
        match_num = 1

        for i in range(0, len(ordered_teams), 2):
            match = self._new_match(f"R1M{match_num}", 0, ordered_teams[i], ordered_teams[i + 1])
            first_round.append(match)
            match_num += 1
        self.bracket.append(first_round)

        round_id = 1
        prev_round = first_round
        while len(prev_round) > 1:
            curr_round = []
            for i in range(0, len(prev_round), 2):
                match_id = f"R{round_id+1}M{(i//2)+1}"
                if len(prev_round) == 2:
                    match_id = "FIRST_PLACE"
                match = self._new_match(
                    match_id,
                    round_id,
                    {"match_id": prev_round[i]["id"], "type": "winner"},
                    {"match_id": prev_round[i + 1]["id"], "type": "winner"},
                )
                curr_round.append(match)
            self.bracket.append(curr_round)
            prev_round = curr_round
            round_id += 1

        if len(self.bracket) >= 2 and len(self.bracket[-2]) == 2:
            semi_1, semi_2 = self.bracket[-2]
            third_match = self._new_match(
                "THIRD_PLACE",
                len(self.bracket) - 1,
                {"match_id": semi_1["id"], "type": "loser"},
                {"match_id": semi_2["id"], "type": "loser"},
            )
            self.bracket[-1].insert(0, third_match)

        self._auto_pass()
        self._sync_done()

    @property
    def winner(self):
        if not self.bracket: return None
        for m in self.bracket[-1]:
            if m["id"] != "THIRD_PLACE":
                return m.get("winner")
        return None

    def is_finished(self):
        return self.finished

    def has_next(self):
        self._auto_pass()
        self._sync_done()
        return self._next_match() is not None

    def _all_matches(self):
        for rounds in self.bracket:
            for match in rounds:
                yield match

    def _get_match(self, match_id):
        for match in self._all_matches():
            if match["id"] == match_id:
                return match
        sys.exit(f"Error: Unknown match id: {match_id}")

    def _read_slot(self, slot):
        if slot is None or isinstance(slot, str):
            return slot
        source = self._get_match(slot["match_id"])
        if slot.get("type", "winner") == "loser":
            return source["loser"]
        return source["winner"]

    def _fill_teams(self, match):
        match["team_a"] = self._read_slot(match["slot_a"])
        match["team_b"] = self._read_slot(match["slot_b"])

    def _sync_done(self):
        self.finished = all(match["played"] for match in self._all_matches())

    def _set_result(self, match_id, winner, auto=False, score=None):
        match = self._get_match(match_id)
        if match["played"]:
            return

        self._fill_teams(match)
        a, b = match["team_a"], match["team_b"]
        loser = b if winner == a else a

        match["winner"] = winner
        match["loser"] = loser
        match["played"] = True
        self.results.append(
            {
                "match_id": match_id,
                "winner": winner,
                "loser": loser,
                "auto": auto,
                "score": score,
            }
        )

    def _auto_pass(self):
        changed = True
        while changed:
            changed = False
            for match in self._all_matches():
                if match["played"]:
                    continue
                self._fill_teams(match)
                a, b = match["team_a"], match["team_b"]
                
                def is_dead_end(slot, val):
                    if val is not None: return False
                    if slot is None: return True
                    if not isinstance(slot, dict): return False
                    source = self._get_match(slot["match_id"])
                    # Retorna True si el partido previo ya se jugó y su resultado dio None.
                    if source["played"]:
                        key = slot.get("type", "winner")
                        if key == "loser": return source.get("loser") is None
                        return source.get("winner") is None
                    return False
                    
                if a is None and b is None:
                    if is_dead_end(match["slot_a"], a) and is_dead_end(match["slot_b"], b):
                        self._set_result(match["id"], None, auto=True)
                        changed = True
                    continue
                    
                if a is not None and is_dead_end(match["slot_b"], b):
                    self._set_result(match["id"], a, auto=True)
                    changed = True
                elif b is not None and is_dead_end(match["slot_a"], a):
                    self._set_result(match["id"], b, auto=True)
                    changed = True

    def _next_match(self):
        for match in self._all_matches():
            if match["played"]:
                continue
            self._fill_teams(match)
            if match["team_a"] is not None and match["team_b"] is not None:
                return match
        return None

    def next_round(self):
        self._auto_pass()
        self._sync_done()
        return self._next_match()

    def run_match(self, match):
        if match is None:
            return None

        team_a = match["team_a"]
        team_b = match["team_b"]

        root = Path(__file__).resolve().parent.parent
        match_round = match["round"]
        levels_dir = self.levels[match_round] if match_round < len(self.levels) else self.levels[-1]
        cmd = [sys.executable, "main.py", team_a, team_b, "--full-screen", "--levels-dir", levels_dir]

        while True:
            proc = subprocess.run(cmd, cwd=root, capture_output=True, text=True, check=False)
            
            error_msg = None
            if proc.returncode != 0:
                error_msg = f"El juego terminó con error (código {proc.returncode}):\n{proc.stderr}"
            else:
                score_res = re.search(r"Final scores:\s*\[(\d+)\s*,\s*(\d+)\]", proc.stdout)
                score_a, score_b = int(score_res.group(1)), int(score_res.group(2))
                if score_a == score_b:
                    error_msg = f"Empate detectado ({score_a}-{score_b})"
                else:
                    winner = team_a if score_a > score_b else team_b
                    self._set_result(match["id"], winner, auto=False, score=[score_a, score_b])
                    self._auto_pass()
                    self._sync_done()
                    return {
                        "match_id": match["id"],
                        "team_a": team_a,
                        "team_b": team_b,
                        "score": [score_a, score_b],
                        "winner": winner,
                    }

            print(f"\n[!] ERROR EN EL PARTIDO {match['id']} ({team_a} vs {team_b})")
            if error_msg:
                print(error_msg)
            
            print("\n¿Qué deseas hacer para resolver esto?")
            print(f"  1) Dar victoria a {team_a}")
            print(f"  2) Dar victoria a {team_b}")
            print("  3) Elegir ganador al azar")
            print("  4) Reiniciar el partido")
            print("  5) Ronda de desempate")
            
            choice = ""
            while choice not in ["1", "2", "3", "4", "5"]:
                choice = input("Elige una opción (1/2/3/4/5): ").strip()
                
            if choice == "5":
                print("\nJugando ronda de desempate...")
                if match_round == len(self.bracket) - 1:
                    tie_round = max(0, match_round - 1)
                else:
                    tie_round = match_round + 1
                    
                tie_levels_dir = self.levels[tie_round] if tie_round < len(self.levels) else self.levels[-1]
                cmd = [sys.executable, "main.py", team_a, team_b, "--full-screen", "--levels-dir", tie_levels_dir]
                continue
                
            if choice == "4":
                print("\nReiniciando el partido...")
                continue                
            if choice == "3":
                winner = random.choice([team_a, team_b])
                score = [1, 0] if winner == team_a else [0, 1]
            else:
                winner = team_a if choice == "1" else team_b
                score = [1, 0] if choice == "1" else [0, 1]
            
            self._set_result(match["id"], winner, auto=False, score=score)
            self._auto_pass()
            self._sync_done()
            
            return {
                "match_id": match["id"],
                "team_a": team_a,
                "team_b": team_b,
                "score": score,
                "winner": winner,
            }

    def print_result(self, result):
        if not result:
            return

        match_id = result["match_id"]
        team_a = result["team_a"]
        team_b = result["team_b"]
        score_a, score_b = result["score"]
        winner = result["winner"]

        print(f"\n{'='*50}")
        print(f"RESULTADO DEL PARTIDO: {match_id}")
        print(f"{'-'*50}")
        print(f" > {team_a}  [{score_a}] - [{score_b}]  {team_b} <")
        print(f"{'-'*50}")
        print(f"GANADOR: {winner.upper()}")
        print(f"{'='*50}\n")

# ----------------------------------------------------------------------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(description="Modo torneo para el TPC3")
    parser.add_argument("config")
    parser.add_argument("--auto", action="store_true")
    return parser.parse_args()

def run_tournament(tournament, auto=False):
    while tournament.has_next():
        print(tournament)
        if not auto:
            input("Press intro to start next round...")
        match = tournament.next_round()
        results = tournament.run_match(match)
        tournament.print_result(results)

    print(tournament)
    print(f"------- >>> GANADOR DEL TORNEO: {tournament.winner} <<< -------")

def main():
    args = parse_args()
    tournament = Tournament.from_config(args.config)
    run_tournament(tournament, auto=args.auto)
    return 0

if __name__ == "__main__":
    sys.exit(main())
