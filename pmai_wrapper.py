#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     30/03/20
##  Last Update :     30/03/20
##  Version     :     1.0
##  Wrapper and tester for c++ pmai.hh 
##  to be used with python 2.7 and pygame
#############################################################################

from ctypes import *
import os
from random import randint
import collections

pmai_libc = None
try:
  pmai_libc = cdll.LoadLibrary( os.path.abspath("modules/pmai.so") )
except:
  pmai_libc = cdll.LoadLibrary( os.path.abspath("pmai.so") )
# pmai_libc.pmPhysic_PyTick.restype = py_object
pmai_libc.pmAI_GetRoomPath.restype = py_object
pmai_libc.pmAI_GetRoomGrid.restype = py_object

class pmAI ():
  def __init__(self):
    self.ai = pmai_libc.pmAI_new()

  def GetRoomGrid(self, room, type):
    answer = pmai_libc.pmAI_GetRoomGrid(self.ai, room, type)
    size_x = answer[0]
    size_y = answer[1]
    grid = []
    for i in xrange(size_x):
      column = []
      for j in xrange(size_y):
        column += [ answer[4][i*size_y + j] ]
      grid += [ column ]
    print grid
    return answer[0], answer[1], answer[2], answer[3], grid

  def GetRoomPath(self, start_id, end_id):
    answer = pmai_libc.pmAI_GetRoomPath(self.ai, start_id, end_id)
    return answer

  def SetRooms(self, rooms):
    print "pmAI.SetRooms() ... add rooms"
    for room in rooms:
      start_pos_x, start_pos_y, size_x, size_y = room.GetPositionForAI()
      id = pmai_libc.pmAI_AddRoom(self.ai, start_pos_x, start_pos_y, size_x, size_y);
      room.ai_id = id

    print "pmAI.SetRooms() ... add rooms connections"
    for room in rooms:
      connected_rooms = room.GetConnectedRoomForAI()
      connected_rooms_c = (c_int * len(connected_rooms))()
      for i, room_id in enumerate(connected_rooms) :
        connected_rooms_c[i] = room_id
      answer = pmai_libc.pmAI_SetRoomConnections(self.ai, room.ai_id, len(connected_rooms), connected_rooms_c);

    pmai_libc.pmAI_PrintRooms(self.ai)

  # TODO delete self.ai
  #def __del__(self):
  # 

  def Tick(self, bullets):
    bullet_positions = (c_int * (len(bullets) * 2))()
    bullet_tiles     = (c_int * (len(bullets)    ))()
    for i, obj in enumerate( bullets ) :
      bullet_positions[ 2*i   ] = int(obj.pos.x)
      bullet_positions[ 2*i+1 ] = int(obj.pos.y)
      bullet_tiles[i] = obj.room_id-1 #FIXME

    # pmAI * ai, int N_bullets, float * bullet_positions, int * bullet_rooms
    pmai_libc.pmAI_Tick(self.ai, len(bullets), bullet_positions, bullet_tiles)
    
    pass

def main():
  print pmai_libc.check();

if __name__ == "__main__" :
  main()



