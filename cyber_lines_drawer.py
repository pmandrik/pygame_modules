#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     08/03/20
##  Last Update :     08/03/20
##  Version     :     1.0
#############################################################################

# wget --no-check-certificate -N --content-disposition https://raw.githubusercontent.com/pmandrik/PMANDRIK_LIBRARY/master/pmlib_v2d.py
# wget --no-check-certificate -N --content-disposition https://raw.githubusercontent.com/pmandrik/PMANDRIK_LIBRARY/master/pmlib_const.py
from PMANDRIK_LIBRARY.pmlib_v2d_py import *
from random import uniform, randint, randrange, shuffle
from copy import copy
import pygame

class CyberLinesDrawer():
  def __init__(self, start_point, end_point, N_lines, direction, rect, deepth, deepth_y=0, rects_to_exclude=[]):
    self.sp = start_point
    self.ep = end_point
    self.deepth = deepth
    self.deepth_x = deepth
    self.deepth_y = deepth
    self.N_lines = N_lines
    self.direction = direction
    self.edge = self.sp + direction * deepth 
    self.done = False
    self.done_full = False

    self.rects_to_exclude = rects_to_exclude
    self.rects_to_exclude_filtered = []
    for r in self.rects_to_exclude :
      if abs( direction.x * direction.y ) < 0.5 and False:
        if min(self.sp.x, self.ep.x) > r.right   : continue
        if max(self.sp.x, self.ep.x) < r.left  : continue
        if min(self.sp.y, self.ep.y) > r.top    : continue
        if max(self.sp.y, self.ep.y) < r.bottom : continue

      if self.direction.x < 0 and self.sp.x < r.left : continue
      if self.direction.x > 0 and self.sp.x > r.right : continue
      if self.direction.y > 0 and self.sp.y < r.top : continue
      if self.direction.y < 0 and self.sp.y > r.bottom : continue

      self.rects_to_exclude_filtered += [ r ]

    self.rect = rect
    if self.rect : 
      tl, bl, tr, br = rect.topleft, rect.bottomleft, rect.topright, rect.bottomright
      self.tl = v2( tl[0], tl[1] )
      self.bl = v2( bl[0], bl[1] )
      self.tr = v2( tr[0], tr[1] )
      self.br = v2( br[0], br[1] )


    self.points = []
    self.speeds = []
    self.color_indexes = []

    self.plates_size = 15
    self.plates_steps_x = (self.deepth - 10) / self.plates_size + 1
    self.plates_steps_y = (self.deepth - 10) / self.plates_size + 1

    if abs( direction.x * direction.y ) < 0.5 : # v2(+-1, 0), v2(0, +-1)
      for i in xrange(N_lines):
        self.points += [ self.sp + (self.ep-self.sp) * i / N_lines ]
        self.speeds += [ copy(direction) ]
        self.color_indexes += [ i ]
      shuffle( self.color_indexes )
    else : 
      self.deepth_x = deepth
      self.deepth_y = deepth_y
      self.plates_steps_x = (self.deepth_x - 10) / self.plates_size + 1
      self.plates_steps_y = (self.deepth_y - 10) / self.plates_size + 1
      self.Tick = self.TickCorner

    self.life = 1

  def DrawPlate(self, screen, sp, ep, i, rsize = 0):
    plates_size = self.plates_size
    if rsize : plates_size = rsize

    # coff_fade_out = (1. + 0.001 * i) / (0.5*self.life + 1)
    
    coff_fade_out = 1. - 0.95*float(self.life) / self.plates_steps_x

    if abs( self.direction.x *  self.direction.y ) > 0.5 :
      coff_fade_out_x = 1. - 0.95* (abs(sp.x - self.sp.x)+10) / self.deepth_x
      coff_fade_out_y = 1. - 0.95* (abs(sp.y - self.sp.y)+10) / self.deepth_y

      coff_fade_out = min(coff_fade_out_x, coff_fade_out_y)

    if coff_fade_out < 0. : coff_fade_out = 1.

    adds = randint(0,10)
    color = ((60 + adds)*coff_fade_out, (60 + adds)*coff_fade_out, (60 + adds)*coff_fade_out)

    width = ep - sp
    if abs(self.direction.x) > 0.5 : width.x = self.direction.x * plates_size
    if abs(self.direction.y) > 0.5 : width.y = self.direction.y * plates_size

    if width.x < 0 : 
      sp.x += width.x
      width.x = abs( width.x )
    if width.y < 0 : 
      sp.y += width.y
      width.y = abs( width.y )

    rect = pygame.Rect( sp.x, sp.y, width.x, width.y )

    if not rsize :
      rect = rect.inflate( randint(0,10), randint(0,10) )

      for r in self.rects_to_exclude_filtered:
        if r.colliderect( rect ) : return

      pygame.draw.rect( screen, color, rect )

      rect1 = rect.move( randint(-2, 2), randint(-2, 2) )
      color = (color[0]*0.98, color[1]*0.98, color[2]*0.98)
      pygame.draw.rect( screen, color, rect1, 4 )

      rect1 = rect.move( randint(-2, 2), randint(-2, 2) )
      color = (color[0]*0.95, color[1]*0.95, color[2]*0.95)
      pygame.draw.rect( screen, color, rect1, 2 )

      rect1 = rect.move( randint(-2, 2), randint(-2, 2) )
      color = (color[0]*0.90, color[1]*0.90, color[2]*0.90)
      pygame.draw.rect( screen, color, rect1, 1 )  
    else :
      pygame.draw.rect( screen, color, rect )
      color = (color[0]*0.98, color[1]*0.98, color[2]*0.98)
      pygame.draw.rect( screen, color, rect, 4 )
      color = (color[0]*0.95, color[1]*0.95, color[2]*0.95)
      pygame.draw.rect( screen, color, rect, 2 )
      color = (color[0]*0.90, color[1]*0.90, color[2]*0.90)
      pygame.draw.rect( screen, color, rect, 1 )  

  def TickCorner(self, screen):
    plates_size = self.plates_size

    N_plates = 2* self.life - 1 # 1 3 5 7 # 
    indexes = [ i for i in xrange(N_plates) ]
    shuffle ( indexes ) 
    self.done = True
    self.done_full = True

    for i in indexes:
      sp = copy( self.sp )
      
      if i < N_plates / 2:
        sp.x += plates_size * (self.life-1) * self.direction.x
        sp.y += plates_size * i * self.direction.y
      else :
        sp.x += plates_size * (i - N_plates / 2) * self.direction.x
        sp.y += plates_size * (self.life-1) * self.direction.y

      if abs(sp.x - self.sp.x) + plates_size + 10 >= self.deepth_x : continue
      if abs(sp.y - self.sp.y) + plates_size + 10 >= self.deepth_y : continue
      self.done = False
      self.done_full = False

      ep = sp + v2(0, plates_size)
      self.DrawPlate( screen, sp, ep, i )
    self.life += 1

  def Tick(self, screen):
    ### ### DRAW PLATES ### ###
    plates_size = self.plates_size
    if (plates_size)*self.life + 10 < self.deepth :
      N_plates = (self.ep-self.sp).Max() / plates_size
      indexes = [ i for i in xrange(N_plates) ]
      shuffle ( indexes ) 
      for i in indexes:
        sp = self.sp + (self.ep-self.sp) * i / N_plates + self.direction * plates_size * (self.life-1)
        ep = sp + (self.ep-self.sp) / N_plates          + self.direction * plates_size * (self.life-1)
        self.DrawPlate( screen, sp, ep, i )
    elif self.deepth - (plates_size)*self.life > 0 and False:
      N_plates = (self.ep-self.sp).Max() / plates_size
      indexes = [ i for i in xrange(N_plates) ]
      shuffle ( indexes ) 
      for i in indexes:
        sp = self.sp + (self.ep-self.sp) * i / N_plates + self.direction * plates_size * (self.life-1)
        ep = sp + (self.ep-self.sp) / N_plates          + self.direction * plates_size * (self.life-1)
        self.DrawPlate( screen, sp, ep, i, self.deepth - (plates_size)*self.life )
    else : self.done = True

    ### ### DRAW LINES ### ###
    next_points = []
    for i, (speed, point, cindex) in enumerate(zip(self.speeds, self.points, self.color_indexes)) :
      npoint =  point + speed 
      if not randint(0, 10) and self.life > 4:
        if abs(speed.x * self.direction.x) < 0.5 and abs(speed.y * self.direction.y) < 0.5 : speed = copy(self.direction)
        else : speed = v2( self.direction.y * randrange(-1, 2, 2), self.direction.x * randrange(-1, 2, 2) )

      dp = point - self.sp
      de = self.edge - self.sp

      if abs(de.x) > 0.5 and abs(dp.x) > abs(de.x) : continue
      if abs(de.y) > 0.5 and abs(dp.y) > abs(de.y) : continue

      if abs(self.direction.x) > 0 :
        npoint.y = min(max(self.tr.y, npoint.y), self.br.y)
      if abs(self.direction.y) > 0 :
        npoint.x = min(max(self.tl.x, npoint.x), self.tr.x)

      skip_point = False
      for r in self.rects_to_exclude_filtered:
        if r.collidepoint( npoint.x, npoint.y ) : 
          skip_point = True
          break
      if skip_point : continue
      
      width = max(1, (2 + 0.1 * cindex) / (0.1*pow(self.life, 0.5) + 1))
      coff_fade_out = (1. + 0.001 * cindex) / (0.5*pow(self.life, 0.40) + 1)
      coff_fade_out = 1.
      if abs(de.x) > 0.5 : coff_fade_out -= 1.1*float(abs(dp.x)) / abs(de.x)
      if abs(de.y) > 0.5 : coff_fade_out -= 1.1*float(abs(dp.y)) / abs(de.y)
      coff_fade_out = max(0., coff_fade_out)

      shift = v2(1 * speed.y, 1*speed.x)


      color = ((60 + cindex)*coff_fade_out, (60 + cindex)*coff_fade_out, (60 + cindex)*coff_fade_out)
      pygame.draw.line( screen, color, npoint.Tuple(), point.Tuple(), int(width) )

      color = ((50 + cindex)*coff_fade_out, (50 + cindex)*coff_fade_out, (50 + cindex)*coff_fade_out)
      pygame.draw.line( screen, color, (npoint+shift).Tuple(), (point+shift).Tuple(), 1 )

      next_points += [ npoint ]
      self.speeds[i] = speed

    self.life += 1
    if self.life > self.deepth * 2 : self.done_full = True
    self.points = next_points

class CyberRoomDrawer:
  def __init__(self, rect, depth_l, depth_u, depth_r, depth_d, rects_to_exclude=[]):
    tl, bl, tr, br = rect.topleft, rect.bottomleft, rect.topright, rect.bottomright
    tl = v2( tl[0], tl[1] )
    bl = v2( bl[0], bl[1] )
    tr = v2( tr[0], tr[1] )
    br = v2( br[0], br[1] )
    self.done = False

    draw_rect = pygame.Rect( rect[0]-depth_l, rect[1]-depth_u, rect[2]+depth_r+depth_l, rect[3]+depth_u+depth_d )

    self.cld_d = CyberLinesDrawer( bl, br, abs(br.x - bl.x )/15, v2(0,  1), draw_rect, depth_d, 0, rects_to_exclude )
    self.cld_u = CyberLinesDrawer( tl, tr, abs(tr.x - tl.x )/15, v2(0, -1), draw_rect, depth_u, 0, rects_to_exclude )
    self.cld_l = CyberLinesDrawer( tl, bl, abs(tl.y - bl.y )/15, v2(-1, 0), draw_rect, depth_l, 0, rects_to_exclude )
    self.cld_r = CyberLinesDrawer( tr, br, abs(tr.y - br.y )/15, v2(1, 0),  draw_rect, depth_r, 0, rects_to_exclude )

    self.cld_br = CyberLinesDrawer( br, None, 10, v2(1, 1), None, depth_r, depth_d, rects_to_exclude )
    self.cld_bl = CyberLinesDrawer( bl, None, 10, v2(-1, 1), None, depth_l, depth_d, rects_to_exclude )
    self.cld_tr = CyberLinesDrawer( tr, None, 10, v2(1, -1), None, depth_r, depth_u, rects_to_exclude )
    self.cld_tl = CyberLinesDrawer( tl, None, 10, v2(-1, -1), None, depth_l, depth_u, rects_to_exclude )

    self.active = [ self.cld_d, self.cld_u, self.cld_l, self.cld_r, self.cld_br, self.cld_bl, self.cld_tr, self.cld_tl ]

  def Tick(self, screen):
    for item in self.active[:]:
      item.Tick(screen)
      if item.done_full : self.active.remove( item )

    self.done = not len( self.active )

if __name__ == "__main__" :
  import pygame
  pygame.init()
  screen = pygame.display.set_mode( (800, 600) )
  clock = pygame.time.Clock()

  pygame.draw.line( screen, (155, 155, 155), v2(0, 50).Tuple(), v2(640, 50).Tuple(), 5 )
  crd = CyberRoomDrawer( pygame.Rect(200,200,400,200), 200,150,100,50 )

  while True :
    pygame.display.flip()


    crd.Tick( screen )

    clock.tick(60)







