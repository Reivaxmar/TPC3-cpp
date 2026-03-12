from src.controller import Controller
import pygame

class Submission(Controller):

    def info(self):
        self.team_name = "Teclado"
        self.look = 1
        self.color = 2

    def behavior(self):
        keys=pygame.key.get_pressed()
        if (keys[pygame.K_LEFT]):
            self.go_left()
        if (keys[pygame.K_RIGHT]):
            self.go_right()
        if (keys[pygame.K_UP]):
            self.jump()
        if (keys[pygame.K_DOWN]):
            self.grab()
        if (keys[pygame.K_k]):
            self.throw_down()
        if (keys[pygame.K_l]):
            self.throw_right()
        if (keys[pygame.K_j]):
            self.throw_left()
