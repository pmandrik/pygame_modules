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

# TODO fix all types
# TODO fix class* to void*
# pmai_libc.pmPhysic_PyTick.restype = py_object
pmai_libc.pmAI_new.restype = c_void_p
pmai_libc.pmAI_AddRoom.restype = c_int
pmai_libc.pmAI_GetRoomPath.restype = py_object
pmai_libc.pmAI_GetRoomGrid.restype = py_object
pmai_libc.pmAI_GetInRoomPath.restype = py_object
pmai_libc.pmAI_GetInRoomGraphs.restype = py_object

# pmai_libc.pmAI_UpdateObjects.argtypes = [c_int, POINTER(c_void_p), POINTER(c_int), POINTER(c_int)]

# static const int GRID_DEAPTH = 10; // number of types
GRID_DEAPTH = 10

class ObjectAI(Structure):
  _fields_ = [("pos_x", c_int), ("pos_y", c_int),
              ("type", c_int), ("value", c_int),
              ("room_id", c_int), ("id", c_int),
              ("grid_value", POINTER(c_int)),
              ("dir_local_max_x", POINTER(c_int)), ("dir_local_max_y", POINTER(c_int)), ("local_max", POINTER(c_int)),
              ("dir_local_min_x", POINTER(c_int)), ("dir_local_min_y", POINTER(c_int)), ("local_min", POINTER(c_int)),
              ("dist_local_max_x", POINTER(c_int)), ("dist_local_max_y", POINTER(c_int)),
              ("dist_local_min_x", POINTER(c_int)), ("dist_local_min_y", POINTER(c_int))]

pmai_libc.pmAI_AddObject.restype = POINTER( ObjectAI )

class pmAIObject ():
  def __init__(self):
    self.object_ai = None

    self.grid_value = (c_int * GRID_DEAPTH)()

    self.dir_local_max_x = (c_int * GRID_DEAPTH)()
    self.dir_local_max_y = (c_int * GRID_DEAPTH)()
    self.local_max = (c_int * GRID_DEAPTH)()
    self.dir_local_min_x = (c_int * GRID_DEAPTH)()
    self.dir_local_min_y = (c_int * GRID_DEAPTH)()
    self.local_min = (c_int * GRID_DEAPTH)()

    # FIXME
    self.dist_local_max_x = (c_int * GRID_DEAPTH)()
    self.dist_local_max_y = (c_int * GRID_DEAPTH)()
    self.dist_local_min_x = (c_int * GRID_DEAPTH)()
    self.dist_local_min_y = (c_int * GRID_DEAPTH)()

class pmAI ():
  def __init__(self, pmTiledMap_ptr):
    self.ai = c_void_p( pmai_libc.pmAI_new( pmTiledMap_ptr ) )
    self.room_ids_for_graph_rebuild = {}

  def AddAIObject(self, obj, grid_type, grid_value):
    obj.object_ai = pmai_libc.pmAI_AddObject(self.ai, grid_type, grid_value,
          byref(obj.grid_value),
          byref(obj.dir_local_max_x), byref(obj.dir_local_max_y), byref(obj.local_max),
          byref(obj.dir_local_min_x), byref(obj.dir_local_min_y), byref(obj.local_min),
          byref(obj.dist_local_max_x), byref(obj.dist_local_max_y),
          byref(obj.dist_local_min_x), byref(obj.dist_local_min_y)
      )
    print "receive room_id = ", obj.object_ai.contents.room_id

  def RemoveAIObject(self, obj ):
    pmAIObject.object_ai = pmai_libc.pmAI_RemoveObject(self.ai, obj.object_ai)

  def UpdateAIObject(self, obj ):
    pmai_libc.UpdateAIObjects( obj.object_ai )
    
  def RebuildRoomGraph(self, room_id):
    self.room_ids_for_graph_rebuild[room_id] = True

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
    # print grid
    return answer[0], answer[1], answer[2], answer[3], grid
  
  def GetRoomGraphs(self, room):
    answer = pmai_libc.pmAI_GetInRoomGraphs(self.ai, room)
    return answer

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

  def Tick(self, objects, bullets):
    print self.room_ids_for_graph_rebuild
    if self.room_ids_for_graph_rebuild :
      print self.room_ids_for_graph_rebuild
      room_ids = self.room_ids_for_graph_rebuild.keys()
      room_ids_c = (c_int * (len(room_ids) ))()
      for i, key in enumerate(room_ids): 
        room_ids_c[ i ] = key-1 # FIXME
      pmai_libc.pmAI_RebuildRoomGraphs(self.ai, len(room_ids), room_ids_c )
      self.room_ids_for_graph_rebuild = {}
    
    object_positions  = (c_int * (len(objects) * 2))()
    targets_positions  = (c_int  * (len(objects) * 2))()
    targets_visibility = (c_bool * (len(objects)  ))()
    object_tiles      = (c_int * (len(objects)    ))()
    object_ais        = (POINTER(ObjectAI) * (len(objects) ))()
    for i, obj in enumerate( objects ) :
      object_positions[ 2*i   ] = int(obj.pos.x)
      object_positions[ 2*i+1 ] = int(obj.pos.y)
      object_tiles[i] = obj.pos_room-1 #FIXME
      object_ais[i]   = obj.object_ai 
      
      if obj.target and obj.check_target_visibility :
        targets_positions[ 2*i   ] = int(obj.target.pos.x)
        targets_positions[ 2*i+1 ] = int(obj.target.pos.y)
      else :
        targets_positions[ 2*i   ] = object_positions[ 2*i   ];
        targets_positions[ 2*i+1 ] = object_positions[ 2*i+1 ];

    bullet_positions = (c_int * (len(bullets) * 2))()
    bullet_speeds    = (c_float * (len(bullets) * 2))()
    bullet_tiles     = (c_int * (len(bullets)    ))()
    for i, obj in enumerate( bullets ) :
      bullet_positions[ 2*i   ] = int(obj.pos.x)
      bullet_positions[ 2*i+1 ] = int(obj.pos.y)
      bullet_speeds[ 2*i   ] = int(obj.speed.x)
      bullet_speeds[ 2*i+1 ] = int(obj.speed.y)
      bullet_tiles[i] = obj.room_id-1 #FIXME

    # pmAI * ai, int N_bullets, float * bullet_positions, int * bullet_rooms
    pmai_libc.pmAI_UpdateObjects(len(objects), object_ais, object_positions, object_tiles)
    pmai_libc.pmAI_Tick(self.ai, len(bullets), bullet_positions, bullet_speeds, bullet_tiles)
    pmai_libc.pmAI_CheckIfTargetsVisible(self.ai, len(objects), object_positions, targets_positions, targets_visibility)
    
    for i, obj in enumerate( objects ) :
      if obj.target and obj.check_target_visibility :
        obj.target_visibility = targets_visibility[i]
  
  def GetInRoomPath(self, room_id, start_pos, target_pos):
    path = [ [target_pos.x, target_pos.y] ]
    path = pmai_libc.pmAI_GetInRoomPath(self.ai, room_id, int(start_pos.x), int(start_pos.y), int(target_pos.x), int(target_pos.y))
    return path

def main():
  print pmai_libc.check();

if __name__ == "__main__" :
  main()



