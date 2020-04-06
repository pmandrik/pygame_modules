#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     08/03/20
##  Last Update :     08/03/20
##  Version     :     1.0
#############################################################################

import copy
from random import randint, randrange, choice

# FloorFiller - class to fill the rectangular grid with indexes i,j
#   __init__()
#     take the size of the indexes space size_x, size_y - i,j indexes will be in [0, size_x), [0, size_y) range
#   Fill(self, room_size_x, room_size_y):
#     return grid[y][x] with size room_size_x, room_size_y filled with indexes i,j
#   FillWithRects(self, room_size_x, room_size_y, tiles_collections=[], dencity):
#     tiles_collections = [ [size_x, size_y, postfix, relative_probability], [size_x, size_y, postfix, relative_probability], ... ]
#     N_tries = room_size_x * room_size_y * dencity / <rooms_square>
#     choice the tiles from tiles_collections according to relative_probability and fill the random available area in the room
#     with rect with size (size_x, size_y) with [i,j,postfix] where i,j indexes are in range [0, size_x), [0, size_y)
#   CombineGrids(self, grid_1, grid_2):
#     return new grid filled with grid_1 items and then with grid_2, grids are expected to have the same y,x sizes

class FloorFiller():
  def DiceDirection(self, direction) :
    if direction :  return 1 - direction, randrange(-1, 2, 2)
    return randint(0, 1), randrange(-1, 2, 2)

  def MovePoint(self, point, direct, size_x, size_y):
    direction, speed = self.DiceDirection(None)
    if direct > -5: direction = direct
    answer_x = point[0] + speed * direction
    answer_y = point[1] + speed * (1 - direction)

    if answer_x < 0           : answer_x = 2
    if answer_x >= size_x     : answer_x = size_x-3
    if answer_y < 0           : answer_y = 2
    if answer_y >= size_y     : answer_y = size_y-3

    return [answer_x, answer_y], direction

  def GetNextPoint(self, point, direction, speed, size_x, size_y):
    if direction < -5:
      direction, speed = self.DiceDirection(None)

    if not randint(0, 5): direction, speed = self.DiceDirection( direction )

    answer_x = point[0] + speed * direction
    answer_y = point[1] + speed * (1 - direction)

    if answer_x < 0           : 
      answer_x = 0
      direction, speed = self.DiceDirection( direction )
    if answer_x >= size_x : 
      answer_x =   size_x-1
      direction, speed = self.DiceDirection( direction )
    if answer_y < 0           : 
      answer_y = 0
      direction, speed = self.DiceDirection( direction )
    if answer_y >= size_y : 
      answer_y = size_y-1
      direction, speed = self.DiceDirection( direction )

    return [answer_x, answer_y], [direction, speed]

  def Smooth(self, point, p, divider):
    if abs( point[0] - p[0] ) < 3 and abs( point[1] - p[1] ) < 3 : return point, divider
    if (abs( point[0] + p[0] ) + abs( point[1] + p[1] )) < 4 : return point, divider
    point[0] += p[0]
    point[1] += p[1]
    return point, divider + 1

  def Fill(self, room_size_x, room_size_y, size_x, size_y ):
    point = [ randint(0, size_x), randint(0, size_y) ]
    points_line = []
    speed_line  = []
    for x in xrange( room_size_x ):
      point, speed = self.GetNextPoint( point, -999, None, size_x, size_y )
      points_line += [ point ]
      speed_line  += [ speed ]
    
    grid = [ points_line ]
    
    for y in xrange( 1, room_size_y ):
      points_line = []
      prev_points_line = grid[y-1]
      for x in xrange( room_size_x ):
        prev_point = prev_points_line[x]
        direction, speed = speed_line[x]
        point, speed_line[x] = self.GetNextPoint( prev_point, direction, speed, size_x, size_y )

        divider = 1
        if x : 
          p = prev_points_line[x-1]
          point, divider = self.Smooth( point, p, divider)
        if x != room_size_x - 1 : 
          p = prev_points_line[x+1]
          point, divider = self.Smooth( point, p, divider)

        point[0] = int( point[0] / divider )
        point[1] = int( point[1] / divider )

        direction = -999
        if x and points_line[x-1][0] == point[0] and points_line[x-1][1] == point[1]:
          point, direction = self.MovePoint( point, direction, size_x, size_y )

        if direction > -1 : direction = 1 - direction
        if prev_point[0] == point[0] and prev_point[1] == point[1]:
          point, direction = self.MovePoint( point, direction, size_x, size_y )

        if direction > -1 : direction = 1 - direction
        if x and points_line[x-1][0] == point[0] and points_line[x-1][1] == point[1]:
          point, direction = self.MovePoint( point, direction, size_x, size_y )

        if x and points_line[x-1][0] == point[0] and points_line[x-1][1] == point[1]: print "1"
        if prev_point[0] == point[0] and prev_point[1] == point[1]: print "2"

        points_line += [ point ]
      grid += [ points_line ]
    return grid

  def FillWithRects(self, room_size_x, room_size_y, tiles_collections, dencity=0.5, mode="def"):
    ### fill grid with nulles
    grid = []
    for y in xrange( room_size_y ):
      line  = [ [] for x in xrange( room_size_x ) ]
      grid += [ line ]

    ### prepare rect collections for sampling with unweighted python 2.7 choice
    if not tiles_collections : return grid
    rects = []
    rooms_square = 0
    for collection in tiles_collections:
      # [size_x, size_y, postfix, relative_probability]
      if len(collection) < 3 : return grid
      probability = 1
      try   : probability = collection[3]
      except: pass
      rects += [ collection for i in xrange(int(probability)) ]
      rooms_square += int(probability) * collection[0] * collection[1]
    rooms_square = float(rooms_square) / len( rects )
    N_tries = int(room_size_x * room_size_y * dencity / rooms_square)
      
    ### fill the grid with rects
    booked_points = {}
    def CheckPoint(start_x, start_y, size_x, size_y):
      for x in xrange(start_x, start_x+size_x):
        for y in xrange(start_y, start_y+size_y):
          if booked_points.has_key( (x, y) ) : return False

      if mode == "connected" : 
        if not booked_points : return True
        for x in xrange(start_x-1, start_x+size_x+1):
          for y in xrange(start_y-1, start_y+size_y+1):
            if x != start_x-1 and x != start_x+size_x and y != start_y-1 and y != start_y+size_y   : continue
            if (x == start_x-1 or x == start_x+size_x) and (y == start_y-1 or y == start_y+size_y) : continue
            if booked_points.has_key( (x, y) ) : return True
        return False

      if mode == "connected_once" : 
        if not booked_points : return True
        connections = 0
        for x in xrange(start_x-1, start_x+size_x+1):
          for y in xrange(start_y-1, start_y+size_y+1):
            if x != start_x-1 and x != start_x+size_x and y != start_y-1 and y != start_y+size_y   : continue
            if (x == start_x-1 or x == start_x+size_x) and (y == start_y-1 or y == start_y+size_y) : continue
            if booked_points.has_key( (x, y) ) : connections += 1
            if connections >= 2 : return False
        return connections

      return True

    for i in xrange(N_tries):
      rect = choice( rects )

      size_x = rect[0]
      size_y = rect[1]
      postfix = rect[2]

      ### find all available spaces
      available_points = []
      for x in xrange(0, room_size_x - size_x):
        for y in xrange(0, room_size_y - size_y):
          if not CheckPoint( x, y, size_x, size_y ): continue
          available_points += [ (x,y) ]

      print i, "available_points", available_points

      ### choice one and fill grid
      if not available_points : continue
      point = choice( available_points )
      
      for i, y in enumerate(xrange( point[1], point[1]+size_y )):
        for j, x in enumerate(xrange( point[0], point[0]+size_x )):
          grid[y][x] = [ j, i, postfix ]
          booked_points[ (x,y) ] = True

    return grid
    
  def CombineGrids(self, grid_1, grid_2):
    grid = copy.deepcopy(grid_1)
    for x, line in enumerate(grid_2) : 
      for y, point in enumerate(line) : 
        if not point : continue
        grid[y][x] = copy.copy(point)
    return grid

  def Print(self, grid):
    for line in grid:
      print "|",
      for point in line : 
        if not point :
          print "N,N", 
          continue
        print str(point[0])+","+str(point[1]),
      print "|"

class WallFiller(): # TODO
  def __init__(self):
    pass

if __name__== "__main__" :
  pallet_size_x = 30
  pallet_size_y = 30
  room_size_x = 30
  room_size_y = 30
  tile_size = 15

  ff = FloorFiller()
  ff.Fill( room_size_x, room_size_y, pallet_size_x, pallet_size_y )

  from pygame import *
  init();
  screen = display.set_mode((room_size_x*tile_size, room_size_y*tile_size));

  clock = time.Clock()
  work = True
  while work:
    clock.tick(10)

    screen.fill( (255, 255, 255) )

    ff = FloorFiller( )
    grid = ff.Fill( room_size_x, room_size_y, pallet_size_x, pallet_size_y )
    rects = [ [randint(1, 4), randint(1, 4), (25*i, 0, 255 - 20 * i), 1+i] for i in xrange(10) ]
    print rects
    grid_rects = ff.FillWithRects( room_size_x, room_size_y, rects, 0.35, "connected_once" ) # "connected"
    grid_combo = ff.CombineGrids(grid, grid_rects)

    #ff.Print( grid )
    ff.Print( grid_rects )
    #ff.Print( grid_combo )

    for y, line in enumerate(grid_combo) :
      for x, point in enumerate(line) : 
        
        coff_x = 0.5 / pallet_size_x * point[0]
        coff_y = 0.5 / pallet_size_y * point[1]
        coff = coff_x + coff_y 
        color = (255 * coff, 255 * coff, 255* coff)
        if len(point) == 3: color = point[2]
        draw.rect( screen, color, [x*tile_size, y*tile_size, tile_size, tile_size]  )
    display.flip()

    raw_input()

  quit()












