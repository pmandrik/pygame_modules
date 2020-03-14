#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     08/03/20
##  Last Update :     08/03/20
##  Version     :     1.0
#############################################################################

from random import randint, randrange

class FloorFiller():
  def __init__(self, size_x, size_y):
    self.size_x, self.size_y = size_x, size_y
    self.grid = []
    self.directions = []

  def DiceDirection(self, direction) :
    if direction :  return 1 - direction, randrange(-1, 2, 2)
    return randint(0, 1), randrange(-1, 2, 2)

  def MovePoint(self, point, direct):
    direction, speed = self.DiceDirection(None)
    if direct > -5: direction = direct
    answer_x = point[0] + speed * direction
    answer_y = point[1] + speed * (1 - direction)

    if answer_x < 0           : answer_x = 1
    if answer_x >= self.size_x : answer_x = self.size_x-3
    if answer_y < 0           : answer_y = 1
    if answer_y >= self.size_y : answer_y = self.size_y-3

    return [answer_x, answer_y], direction

  def GetNextPoint(self, point, direction, speed):
    if direction < -5:
      direction, speed = self.DiceDirection(None)

    if not randint(0, 5): direction, speed = self.DiceDirection( direction )

    answer_x = point[0] + speed * direction
    answer_y = point[1] + speed * (1 - direction)

    if answer_x < 0           : 
      answer_x = 0
      direction, speed = self.DiceDirection( direction )
    if answer_x >= self.size_x : 
      answer_x = self.size_x-1
      direction, speed = self.DiceDirection( direction )
    if answer_y < 0           : 
      answer_y = 0
      direction, speed = self.DiceDirection( direction )
    if answer_y >= self.size_y : 
      answer_y = self.size_y-1
      direction, speed = self.DiceDirection( direction )

    return [answer_x, answer_y], [direction, speed]

  def Smooth(self, point, p, divider):
    if abs( point[0] - p[0] ) < 3 and abs( point[1] - p[1] ) < 3 : return point, divider
    if (abs( point[0] + p[0] ) + abs( point[1] + p[1] )) < 4 : return point, divider
    point[0] += p[0]
    point[1] += p[1]
    return point, divider + 1

  def Fill(self, room_size_x, room_size_y):
    point = [ randint(0, self.size_x), randint(0, self.size_y) ]
    points_line = []
    speed_line  = []
    for x in xrange( room_size_x ):
      point, speed = self.GetNextPoint( point, -999, None )
      points_line += [ point ]
      speed_line  += [ speed ]
    
    self.grid += [ points_line ]
    
    for y in xrange( 1, room_size_y ):
      points_line = []
      prev_points_line = self.grid[y-1]
      for x in xrange( room_size_x ):
        prev_point = prev_points_line[x]
        direction, speed = speed_line[x]
        point, speed_line[x] = self.GetNextPoint( prev_point, direction, speed )

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
          point, direction = self.MovePoint( point, direction )

        if direction > -1 : direction = 1 - direction
        if prev_point[0] == point[0] and prev_point[1] == point[1]:
          point, direction = self.MovePoint( point, direction )

        if direction > -1 : direction = 1 - direction
        if x and points_line[x-1][0] == point[0] and points_line[x-1][1] == point[1]:
          point, direction = self.MovePoint( point, direction )

        if x and points_line[x-1][0] == point[0] and points_line[x-1][1] == point[1]: print "1"
        if prev_point[0] == point[0] and prev_point[1] == point[1]: print "2"

        points_line += [ point ]
      self.grid += [ points_line ]

  def Print(self):
    for line in self.grid:
      print "|",
      for point in line : 
        print str(point[0])+","+str(point[1]),
      print "|"

class WallFiller():
  def __init__(self):
    pass

if __name__== "__main__" :
  pallet_size_x = 30
  pallet_size_y = 30
  room_size_x = 30
  room_size_y = 30
  tile_size = 15

  ff = FloorFiller( pallet_size_x, pallet_size_y )
  ff.Fill( room_size_x, room_size_y )

  from pygame import *
  init();
  screen = display.set_mode((room_size_x*tile_size, room_size_y*tile_size));

  clock = time.Clock()
  work = True
  while work:
    clock.tick(10)

    screen.fill( (255, 255, 255) )

    ff = FloorFiller( pallet_size_x, pallet_size_y )
    ff.Fill( room_size_x, room_size_y )
    ff.Print()

    for y, line in enumerate(ff.grid) :
      for x, point in enumerate(line) : 
        coff_x = 0.5 / pallet_size_x * point[0]
        coff_y = 0.5 / pallet_size_y * point[1]
        coff = coff_x + coff_y 
        color = (255 * coff, 255 * coff, 255* coff)
        draw.rect( screen, color, [x*tile_size, y*tile_size, tile_size, tile_size]  )
    display.flip()

    raw_input()

  quit()












