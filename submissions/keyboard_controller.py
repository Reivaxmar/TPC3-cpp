from src.controller import Controller
import pygame

class Submission(Controller):

    def info(self):
        self.team_name = "Teclado"
        self.look = 1
        self.color = 2

    def behavior(self):
        keys=pygame.key.get_pressed()
        if (keys[pygame.K_a]):
            self.go_left()
        if (keys[pygame.K_d]):
            self.go_right()
        if (keys[pygame.K_w]):
            self.jump()
        if (keys[pygame.K_s]):
            self.grab()
        if (keys[pygame.K_g]):
            self.throw_down()
        if (keys[pygame.K_h]):
            self.throw_right()
        if (keys[pygame.K_f]):
            self.throw_left()
