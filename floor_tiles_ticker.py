#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     10/03/20
##  Last Update :     10/03/20
##  Version     :     1.0
#############################################################################

from random import uniform

class FloorTicker():
  def __init__(self, room_size_x, room_size_y, start_point_x, start_point_y, mode="def"):
    self.sx, self.sy = room_size_x, room_size_y
    self.spx, self.spy = start_point_x, start_point_y
    self.drawn_tiles     = []
    self.new_drawn_tiles = []
    self.done = False

    if mode == "alt":
      self.Tick = self.Tick_Alt
    self.step = 0

  def TickWrapper(function):
    def wrapper(self):
        self.step += 1
        if not self.new_drawn_tiles and not self.drawn_tiles:
          self.new_drawn_tiles = [ [self.spx, self.spy] ] 
          return self.new_drawn_tiles
        return function(self)
    return wrapper

  @TickWrapper
  def Tick(self):
    def add_posses( pos, dir, steps):
      answer = []
      for i in xrange( steps ):
        x = self.spx +pos[0] + dir[0]*i
        if x < 0 or x >= self.sx : continue
        y = self.spy + pos[1] + dir[1]*i
        if y < 0 or y >= self.sy : continue
        answer += [ (x, y) ]
      return answer

    nposes = []
    step = self.step-1
    nposes += add_posses( [0, step], [1, -1],  self.step )
    nposes += add_posses( [step, 0], [-1, -1], self.step )
    nposes += add_posses( [0, -step], [-1, 1], self.step )
    nposes += add_posses( [-step, 0], [1, 1],  self.step )

    self.drawn_tiles += self.new_drawn_tiles
    self.new_drawn_tiles = nposes
    if not len(self.new_drawn_tiles) : self.done = True
    return set(self.new_drawn_tiles)
    
  @TickWrapper
  def Tick_Alt(self):
    def add_posses( pos, dir, steps):
      answer = []
      for i in xrange( steps ):
        x = self.spx +pos[0] + dir[0]*i
        if x < 0 or x >= self.sx : continue
        y = self.spy + pos[1] + dir[1]*i
        if y < 0 or y >= self.sy : continue
        answer += [ (x, y) ]
      return answer

    nposes = []
    step = self.step-1
    nposes += add_posses( [-step, step],  [1, 0],  step*2 )
    nposes += add_posses( [step, step],   [0, -1], step*2 )
    nposes += add_posses( [step, -step],  [-1, 0], step*2 )
    nposes += add_posses( [-step, -step], [0, 1],  step*2 )

    self.drawn_tiles += self.new_drawn_tiles
    self.new_drawn_tiles = nposes
    if not len(self.new_drawn_tiles) : self.done = True
    return set(self.new_drawn_tiles)
    pass

if __name__== "__main__" :
  pallet_size_x = 30
  pallet_size_y = 30
  room_size_x = 30
  room_size_y = 30
  tile_size = 15

  ft = FloorTicker( room_size_x, room_size_y, room_size_x/2+10, room_size_y/2+10, "alt" )

  from pygame import *
  init();
  screen = display.set_mode((room_size_x*tile_size+300, room_size_y*tile_size+300));

  clock = time.Clock()
  work = True
  screen.fill( (255, 255, 255) )

  while work:
    clock.tick(10)

    items_to_draw = ft.Tick()
    print items_to_draw

    for point in items_to_draw : 
      x, y = point[0], point[1]
      coff = uniform(0.25, 0.75)
      color = (255 * coff, 255 * coff, 255* coff)
      draw.rect( screen, color, [150 + x*tile_size, 150 + y*tile_size, tile_size, tile_size]  )
    display.flip()

    # raw_input()

  quit()












