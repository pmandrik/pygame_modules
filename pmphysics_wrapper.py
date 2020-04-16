#############################################################################
##  Author      :     P.S. Mandrik, IHEP
##  Date        :     19/03/20
##  Last Update :     19/03/20
##  Version     :     1.0
##  Wrapper and tester for c++ pmphysic.hh 
##  to be used with python 2.7 and pygame
#############################################################################

from ctypes import *
import os
from random import randint
import collections

# TODO fix all types
pmphysics_libc = cdll.LoadLibrary( os.path.abspath("modules/pmphysics.so") )
pmphysics_libc.pmPhysic_new.restype = c_void_p
pmphysics_libc.pmTiledMap_new.restype = c_void_p
pmphysics_libc.pmPhysic_PyTick.restype = py_object

class pmPhysic ():
  def __init__(self, msx, msy, mst, ts):
    self.map_size_x = msx
    self.map_size_y = msy
    self.map_size_types = mst
    self.map_size_y_X_map_size_types = msy * mst
    self.tile_size = ts
    self.map_data = (c_int * ((self.map_size_x*self.map_size_y*self.map_size_types) + 1))()
    
    self.pmPhysic_ptr   = None
    self.pmTiledMap_ptr = None

  def __del__(self):
    if self.pmPhysic_ptr   : pmphysics_libc.pmPhysic_del( self.pmPhysic_ptr )
    if self.pmTiledMap_ptr : pass #FIXME

  def UpdateMap(self, x, y, type, value):
    if x < 0 or x >= self.map_size_x : return
    if y < 0 or y >= self.map_size_y : return
    if type < 0 or type >= self.map_size_types : return
    self.map_data[ self.map_size_y_X_map_size_types * x + self.map_size_types * y + type ] = value
    
  def GetMapValueSafe(self, x, y, type):
    x = int(x / self.tile_size)
    y = int(y / self.tile_size)
    if x < 0 or x >= self.map_size_x : return -1
    if y < 0 or y >= self.map_size_y : return -1
    if type < 0 or type >= self.map_size_types : return -1
    return self.map_data[ self.map_size_y_X_map_size_types * x + self.map_size_types * y + type ]

  def PrintMap(self):
    if not self.pmTiledMap_ptr : return
    pmphysics_libc.pmTiledMap_PrintMap( self.pmTiledMap_ptr, 0 )

  def InitMapFrom3dArray( self, map_data_array ):
    if True : 
      print "python.pmPhysic.InitMapFrom3dArray():"
      if self.map_size_x != len(map_data_array): "expected map X size", self.map_size_x, "not equal to given one", len(map_data_array)
      if self.map_size_y != len(map_data_array[0]): "expected map Y size", self.map_size_x, "not equal to given one", len(map_data_array[0])
      if self.map_size_types != len(map_data_array[0][0]): "expected map Z size", self.map_size_x, "not equal to given one", len(map_data_array[0][0])

      map_size_xy = self.map_size_y*self.map_size_types
      for x in xrange(self.map_size_x) :
        for y in xrange(self.map_size_y) :
          for t in xrange(self.map_size_types) :
            # print "index", x*map_size_xy + y*self.map_size_types + t, x, y, t
            # print self.map_data[ x*map_size_xy + y*self.map_size_types + t ]
            # print map_data_array[x][y][t]
            self.map_data[ x*map_size_xy + y*self.map_size_types + t ] = map_data_array[x][y][t]
    
    self.pmTiledMap_ptr = c_void_p( pmphysics_libc.pmTiledMap_new( self.map_size_x, self.map_size_y, self.map_size_types, self.tile_size, byref(self.map_data) ) )
    self.physic = c_void_p( pmphysics_libc.pmPhysic_new( self.pmTiledMap_ptr) )
    self.PrintMap()

  def Tick(self, objects, bullets):
    # PyObject* pmPhysic_PyTick(pmPhysic* physic, int N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_r_sizes, int N_bullets, float * bullet_positions, int * bullet_tiles)

    object_positions = (c_float * (len(objects) * 2))()
    object_speeds = (c_float * (len(objects) * 2))()
    object_tiles   = (c_int * (len(objects) * self.map_size_types))()
    object_sizes_xy = (c_float * (len(objects) * 2))()  
    object_angles   = (c_float * len(objects))()
    for i, obj in enumerate( objects ) :
      object_positions[ 2*i   ] = obj.pos.x
      object_positions[ 2*i+1 ] = obj.pos.y
      object_speeds[ 2*i   ] = obj.speed.x + obj.inertia.x + obj.inertia_short.x
      object_speeds[ 2*i+1 ] = obj.speed.y + obj.inertia.y + obj.inertia_short.y
      object_sizes_xy[ 2*i   ] = obj.size.x
      object_sizes_xy[ 2*i+1 ] = obj.size.y
      object_angles[i] = obj.angle
      
    bullet_positions = (c_float * (len(bullets) * 2))()
    bullet_tiles = (c_int * (len(bullets) * self.tile_size))()
    for i, obj in enumerate( bullets ) :
      bullet_positions[ 2*i   ] = obj.pos.x
      bullet_positions[ 2*i+1 ] = obj.pos.y

    result = None
    # print "!!! 1"
    result = pmphysics_libc.pmPhysic_PyTick( self.physic, len(objects), byref(object_positions), byref(object_speeds), byref(object_tiles), object_sizes_xy, object_angles, len(bullets), byref(bullet_positions), byref(bullet_tiles) )
    # print "!!! 2"
    
    for i, obj in enumerate( objects ) :
      obj.pos.x   = object_positions[ 2*i   ] 
      obj.pos.y   = object_positions[ 2*i+1 ] 
      obj.speed.x = object_speeds[ 2*i   ]
      obj.speed.y = object_speeds[ 2*i+1 ]

      index = i*self.map_size_types
      for type in xrange(self.map_size_types):
        obj.map_tiles[type] = object_tiles[index + type]

    for i, obj in enumerate( bullets ) :
      #FIXME
      obj.map_tiles = [0]*self.map_size_types
      index = i*self.map_size_types
      for type in xrange(self.map_size_types):
        obj.map_tiles[type] = bullet_tiles[index + type]

    object_grid_pairs, bullet_grid_pairs = result

    ### by constructions, the bullets_x_objects are sorted by bullets indexes first then by objects
    bullet_x_object = {}
    prev_id = -1
    prev_list = set()
    for bullet_id, object_id in zip(*[iter(bullet_grid_pairs)]*2):
      # print "b, o, ids", bullet_id, object_id
      if prev_id != bullet_id and prev_list:
        bullet_x_object[prev_id] = prev_list
        prev_list = set()

      prev_list.add( objects[object_id] )
      prev_id = bullet_id
    bullet_x_object[prev_id] = prev_list

    # print "exit ... "
    return result, object_positions, object_speeds, object_tiles, bullet_positions, bullet_tiles, bullet_x_object

def main():
  pmphysics_libc = cdll.LoadLibrary( os.path.abspath("pmphysics.so") )
  pmphysics_libc.pmPhysic_new.argtypes = [c_int, c_int, c_int, c_int, POINTER(c_int)]

  print pmphysics_libc.check()

  print pmphysics_libc.__dict__
  print pmphysics_libc.pmPhysic_new

  map_size_x = 50
  map_size_y = 50
  map_size_types = 3
  tile_size = 10
  map_data = (c_int * ((map_size_x*map_size_y*map_size_types) + 1))()
  for x in xrange(map_size_x):
    for y in xrange(map_size_y):
      map_data[ (map_size_y*map_size_types)*x + map_size_types * y ] = randint(0, 1);
  # physic = pmphysics_libc.pmPhysic_new( map_size_x  )
  physic = pmphysics_libc.pmPhysic_new(map_size_x, map_size_y, map_size_types, tile_size, map_data)

  pmphysics_libc.pmPhysic_PrintMap( physic, 0 )

if __name__ == "__main__" :
  main()



