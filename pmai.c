
#ifndef pmai
#define pmai 1

// g++ -Wall -Wno-long-long -I/usr/include/python2.7 -lpython2.7  -Wextra -O -ansi -pedantic -shared -o pmai.so -fPIC pmai.c

#include <Python.h>
#include <vector>
#include <map>
#include <cmath>

#define DEBUG_CODE 1 

#ifdef DEBUG_CODE 
#include <iostream>
#define DPRINT(x) (x)
#else 
#define DPRINT(x) do{}while(0)
#endif

static const int GRID_SIZE   =  4*15;
static const int GRID_SIZE_2 = GRID_SIZE/2;

static const int GRID_DEAPTH = 10; // number of types
static const int BULLET_GRID_TYPE = 0; // zero is reserved for bullets
static const int PLAYER_TEAM_GRID_TYPE = 1; // 
static const int ENEMY_TEAM_GRID_TYPE  = 2; // 
static const int ENEMY_TEAM2_GRID_TYPE = 3; // 
static const int ENEMY_TEAM3_GRID_TYPE = 4; // 
static const int DANGEROUS_OBJECTS     = 5; // 
static const int HELPFULL_OBJECTS      = 6; // 

// structute to be used to store info about rooms
struct room_info{
  room_info(const int & px, const int & py, const int & sx, const int & sy){
    start_x = px;
    start_y = py;
    size_x = sx;
    size_y = sy;
    center_x = start_x + size_x/2;
    center_y = start_y + size_y/2;
    grid_size_x = size_x / GRID_SIZE;
    grid_size_y = size_y / GRID_SIZE;
    if(size_x % GRID_SIZE) grid_size_x++;
    if(size_y % GRID_SIZE) grid_size_y++;

    printf( "room_info \n");
    grid = new int ** [grid_size_x]; // [grid_size_x][grid_size_y][GRID_DEAPTH];
    grid_back = new int ** [grid_size_x]; // [grid_size_x][grid_size_y][GRID_DEAPTH];
    for(int i = 0; i < grid_size_x; i++){
      grid[i] = new int * [grid_size_y];
      grid_back[i] = new int * [grid_size_y];
      for(int j = 0; j < grid_size_y; j++){
        grid[i][j] = new int [ GRID_DEAPTH ];
        grid_back[i][j] = new int [ GRID_DEAPTH ];
      }
    }

    ResetGrid();

    //        GSS, SSF, SFF
    // Tune 1   2,   2,   7

    grid_spline_steps   = 1;
    grid_spline_factor  = 2;
    grid_fadeout_factor = 10; // 4 for 1, 6 for 2
    /*
grid_spline_steps  = 2;
grid_spline_factor = 2;
sum = 1.
for i in xrange(-grid_spline_steps, grid_spline_steps+1, 1):
  for j in xrange(-grid_spline_steps, grid_spline_steps+1, 1):
    if i == 0 and j == 0 : continue;
    sum += 1. / ( abs(grid_spline_factor * i) + abs(grid_spline_factor * j) )

print sum
    */
  }

  int id;
  std::vector<int> connected_rooms;
  int start_x, start_y;
  int size_x, size_y;
  int center_x, center_y;
  int grid_size_x, grid_size_y;
  int *** grid;
  int *** grid_back;
  int grid_spline_steps, grid_spline_factor, grid_fadeout_factor;
  int maximums[GRID_DEAPTH], max_x[GRID_DEAPTH], max_y[GRID_DEAPTH];
  int minimums[GRID_DEAPTH], min_x[GRID_DEAPTH], min_y[GRID_DEAPTH];

  void ResetGrid(){
    for(int k = 0; k < GRID_DEAPTH; k++){
      for(int i = 0; i < grid_size_x; i++)
        for(int j = 0; j < grid_size_y; j++)
          grid[i][j][k] = 0;
      maximums[k] = 0;
      minimums[k] = 0;
      min_x[k] = 0;
      min_y[k] = 0;
      max_x[k] = 0;
      max_y[k] = 0;
    }
  }

  void FadeOutGrid(){
    for(int k = 0; k < GRID_DEAPTH; k++){
      for(int i = 0; i < grid_size_x; i++)
        for(int j = 0; j < grid_size_y; j++){
          //grid[i][j][k] -= 1;
          //grid[i][j][k] -= grid[i][j][k]/10;
          grid[i][j][k] /= grid_fadeout_factor;
          grid[i][j][k] = std::max(0, grid[i][j][k]);
          grid[i][j][k] = std::min(5000, grid[i][j][k]);
        }
      maximums[k] = 0;
      minimums[k] = 0;
      min_x[k] = 0;
      min_y[k] = 0;
      max_x[k] = 0;
      max_y[k] = 0;
    }
  }

  void SetMinMaxPositions(const int & type, float * min_x, float * min_y, float * max_x, float * max_y, float * max_value, float * min_value){
    *min_x = start_x + min_x[ type ] * GRID_SIZE - 0.5 * GRID_SIZE;
    *min_y = start_y + min_y[ type ] * GRID_SIZE - 0.5 * GRID_SIZE;
    *min_value = minimums[ type ];

    *max_x = start_x + max_x[ type ] * GRID_SIZE - 0.5 * GRID_SIZE;
    *max_y = start_y + max_y[ type ] * GRID_SIZE - 0.5 * GRID_SIZE;
    *max_value = maximums[ type ];
  }

  void CheckDirection(const int & type, const int & x, const int & y, int & mmax, int & mmin, int & dx_max, int & dy_max, int & dx_min, int & dy_min){
    if(x < 0 or x >= grid_size_x or y < 0 or y >= grid_size_y or type < 0 or type >= GRID_DEAPTH) return;
    if( grid[x][y][type] > mmax ){
      mmax = grid[x][y][type];
      dx_max = x;
      dy_max = y;
    }
    if( grid[x][y][type] < mmin ){
      mmin = grid[x][y][type];
      dx_min = x;
      dy_min = y;
    }
    return;
  }

  void SetLocalDirections(const int & type, const int & px, const int & py, int & dx_max, int & dy_max, int & dx_min, int & dy_min, int & mmax, int & mmin){
    int x = (px - start_x) / GRID_SIZE;
    int y = (py - start_y) / GRID_SIZE;
    if(x < 0 or x >= grid_size_x or y < 0 or y >= grid_size_y or type < 0 or type >= GRID_DEAPTH) return;

    // DPRINT( printf("SetLocalDirections() = %d %d %d %d \n", x, y, grid_size_x, grid_size_y) );
    mmax = grid[x][y][type];
    mmin = grid[x][y][type];
    dx_max = x;
    dy_max = y;
    dx_min = x;
    dy_min = y;

    CheckDirection(type, x,   y-1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x,   y+1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x-1, y,   mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x-1, y-1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x-1, y+1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x+1, y,   mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x+1, y-1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    CheckDirection(type, x+1, y+1, mmax, mmin, dx_max, dy_max, dx_min, dy_min);
    
    int shift_from_center_x = (px - start_x) % GRID_SIZE - GRID_SIZE_2;
    int shift_from_center_y = (py - start_y) % GRID_SIZE - GRID_SIZE_2;
    dx_max = (dx_max - x)*GRID_SIZE + shift_from_center_x;
    dy_max = (dy_max - y)*GRID_SIZE + shift_from_center_y;
    dx_min = (dx_min - x)*GRID_SIZE + shift_from_center_x;
    dy_min = (dy_min - y)*GRID_SIZE + shift_from_center_y;
  }

  int GetGridValueSafe(const int & px, const int & py, const int & type){
    int x = (px - start_x) / GRID_SIZE;
    int y = (py - start_y) / GRID_SIZE;
    if(x < 0 or x >= grid_size_x or y < 0 or y >= grid_size_y or type < 0 or type >= GRID_DEAPTH){
      DPRINT( printf("room_info.GetGridValueSafe(): invalid grid request %d %d %d for grid %d %d %d %d \n", px, py, type, grid_size_x, grid_size_y, GRID_SIZE, GRID_DEAPTH) );
      return 0;
    }
    return grid[x][y][type];
  }

  bool AddToGridSafe(const int & px, const int & py, const int & type, const int & value){
    int x = (px - start_x) / GRID_SIZE;
    int y = (py - start_y) / GRID_SIZE;
    if(x < 0 or x >= grid_size_x or y < 0 or y >= grid_size_y or type < 0 or type >= GRID_DEAPTH){
      DPRINT( printf("room_info.AddToGridSafe(): invalid grid request %d %d %d for grid %d %d %d %d %d \n", px, py, type, start_x, start_y, start_x+grid_size_x*GRID_SIZE, start_y+grid_size_y*GRID_SIZE, GRID_DEAPTH) );
      return false;
    }
    grid[x][y][type] += value;
    return true;
    //if( grid[x][y][type] > maximums[type] ){
    //  maximums[type] = grid[x][y][type];
    //  max_x[type] = x;
    //  max_y[type] = y;
    //}
  }

  void PostProcess(){
    for(int k = 0; k < GRID_DEAPTH; k++){
      for(int x = 0; x < grid_size_x; x++){
        for(int y = 0; y < grid_size_y; y++){
          if(not k and not x and not y)
            DPRINT( printf("room_info.PostProcess %d %d %d\n", id, grid_back[x][y][k], grid[x][y][k]) );
          grid_back[x][y][k] = grid[x][y][k];
          for(int dx = -grid_spline_steps; dx <= grid_spline_steps; dx++)
            for(int dy = -grid_spline_steps; dy <= grid_spline_steps; dy++){
              if( dx == 0 and dy == 0 ) continue;
              if( (x + dx) < 0 or (y + dy) < 0 or (x + dx) >= grid_size_x or (y + dy) >= grid_size_y )
                grid_back[x][y][k] += grid[x][y][k] / (grid_spline_factor*std::abs(dx) + grid_spline_factor*std::abs(dy));
              else 
                grid_back[x][y][k] += grid[x+dx][y+dy][k] / (grid_spline_factor*std::abs(dx) + grid_spline_factor*std::abs(dy));
            }
        }
      }
    }

    std::swap(grid, grid_back);

    for(int k = 0; k < GRID_DEAPTH; k++){
      for(int i = 0; i < grid_size_x; i++)
        for(int j = 0; j < grid_size_y; j++){
          if( grid[i][j][k] < minimums[k] ){
            minimums[k] = grid[i][j][k];
            min_x[k] = i;
            min_y[k] = j;
          }
          if( grid[i][j][k] > maximums[k] ){
            maximums[k] = grid[i][j][k];
            max_x[k] = i;
            max_y[k] = j;
          }
        }
    }
  }
};

// structute to be used for path finding between rooms
  struct room_astar_member{
    room_astar_member(room_astar_member * fa = NULL, int id_ = -1, float dte = -1.f, float dts = -1.f){
      father           = fa;
      id               = id_;
      distance_to_end     = dte;
      distance_from_start = dts;
    }

    room_astar_member * father;
    int id;
    int distance_to_end;
    int distance_from_start;
  };


class ObjectAI {
  public:
  ObjectAI(const int & grid_type, const int & value_,
    int *grid_value_,
    int *dir_local_max_x_, int *dir_local_max_y_, int *local_max_,
    int *dir_local_min_x_, int *dir_local_min_y_, int *local_min_,
    int *dist_local_max_x_, int *dist_local_max_y_, int *dist_local_min_x_, int *dist_local_min_y_
  ){
    type = grid_type;
    value = value_;

    grid_value = grid_value_;

    dir_local_max_x = dir_local_max_x_;
    dir_local_max_y = dir_local_max_y_;
    local_max = local_max_;
    dir_local_min_x = dir_local_min_x_;
    dir_local_min_y = dir_local_min_y_;
    local_min = local_min_;

    dist_local_max_x = dist_local_max_x_;
    dist_local_max_y = dist_local_max_y_;
    dist_local_min_x = dist_local_min_x_;
    dist_local_min_y = dist_local_min_y_;

    pos_x = 0;
    pos_y = 0;
  }

  void Update(const int & pos_x_, const int & pos_y_, const int & room_id_){
    pos_x = pos_x_;
    pos_y = pos_y_;
    room_id = room_id_;
  }

  int pos_x, pos_y;
  int type, value;
  int room_id, id;

  int *grid_value;

  int *dir_local_max_x, *dir_local_max_y, *local_max;
  int *dir_local_min_x, *dir_local_min_y, *local_min;

  int *dist_local_max_x, *dist_local_max_y;
  int *dist_local_min_x, *dist_local_min_y;

/*
  int dir_local_max_x[GRID_DEAPTH], dir_local_max_y[GRID_DEAPTH], local_max[GRID_DEAPTH];
  int dir_local_min_x[GRID_DEAPTH], dir_local_min_y[GRID_DEAPTH], local_min[GRID_DEAPTH];

  int dist_local_max_x[GRID_DEAPTH], dist_local_max_y[GRID_DEAPTH];
  int dist_local_min_x[GRID_DEAPTH], dist_local_min_y[GRID_DEAPTH];
*/
};

class pmAI {
  public:
    pmAI(){
      printf("pmai.pmAI() ... ");
      max_astars_per_tick = 5;
      id_counter = 0;
      printf("ok\n");
      raytracer_bullets_steps = 5;
    }

    void Tick(const int & N_bullets, int * bullet_positions, float * bullet_speeds, int * bullet_rooms){
      DPRINT( std::cout << "pmAI.Tick with " << N_bullets << "bullets" << std::endl );
      for(unsigned int i = 0; i < rooms.size(); ++i)
        //rooms[i].ResetGrid();
        rooms[i].FadeOutGrid();

      // AI local
      // calculate the contribution of bullets to the rooms grid
      float pos_x, pos_y;
      float speed_x, speed_y;
      for(int i = 0, i_max = 2*N_bullets; i < i_max; i+=2){
        room_info & room = rooms[ bullet_rooms[i/2] ]; // FIXME
        pos_x = bullet_positions[ i ];
        pos_y = bullet_positions[ i+1 ];
        speed_x = bullet_speeds[ i ];
        speed_y = bullet_speeds[ i+1 ];

        DPRINT( printf("pmAI_Tick bullet %d %f %f %f %f \n", bullet_rooms[i/2], pos_x, pos_y, speed_x, speed_y) );
        room.AddToGridSafe(pos_x, pos_y, BULLET_GRID_TYPE, 100);

        for(int step = 0; step < raytracer_bullets_steps; step++ ){
          pos_x += speed_x*10;
          pos_y += speed_y*10;
          if( not room.AddToGridSafe(pos_x, pos_y, BULLET_GRID_TYPE, 100-10*raytracer_bullets_steps) ) break;
        }
      }
      // calculate the contribution of objects to the rooms grid
      for(std::map<unsigned int, ObjectAI*>::iterator it = objects.begin(); it != objects.end(); ++it){
        ObjectAI * object = it->second;
        // room_info & room = rooms[ object->room_id ]; FIXME
        // room.AddToGridSafe(object->pos_x, object->pos_y, object->type, object->value);
      }

      // find rooms grid maximums and minimums
      // apply smoothing
      for(unsigned int i = 0; i < rooms.size(); ++i)
        rooms[i].PostProcess();

      // calculate the direction to nearests maximums, minimums
      DPRINT( printf("calculate the direction to nearests maximums, minimums\n") );
      for(std::map<unsigned int, ObjectAI*>::iterator it = objects.begin(); it != objects.end(); ++it){
        ObjectAI * object = it->second;
        DPRINT( printf("process object with id, room id = %d %d \n", object->id, object->room_id) );
        room_info & room = rooms[ object->room_id ]; // FIXME

        for(int type = 0; type < GRID_DEAPTH; type++){
          // calculate the direction to nearests maximums, minimums
          room.SetLocalDirections(type, object->pos_x, object->pos_y, 
                                        object->dir_local_max_x[type], object->dir_local_max_y[type],
                                        object->dir_local_min_x[type], object->dir_local_min_y[type], 
                                        object->local_max[type],       object->local_min[type] );
          // distance to global maximums, minimums
          // *dist_local_max_x, *dist_local_max_y;
          // *dist_local_min_x, *dist_local_min_y;
        }

        DPRINT( printf("pmAI_Tick Objects %d %d %d %d %d \n", object->room_id, object->pos_x, object->pos_y, object->dir_local_min_x[0], object->dir_local_min_y[0]) );

        // set the value for current objects position
        for(int type = 0; type < GRID_DEAPTH; type++){
          object->grid_value[ type ] = room.GetGridValueSafe(object->pos_x, object->pos_y, type);
        }
      }

      // get path [point sequences] to the target within the room

      // check if target is visible

      // AI global
      done_astars_per_tick = 0;
      // get path [rooms ids] to target
      
      // 
      DPRINT(std::cout << "pmAI.Tick done ..." << std::endl );
    }

  int AddRoom(int px, int py, int sx, int sy){
    rooms.push_back( room_info(px, py, sx, sy) );
    rooms[rooms.size()-1].id = rooms.size()-1;
    return rooms.size()-1;
  }

  bool SetRoomConnections(int room_id, int n_connected_rooms, int * connected_rooms_ids){
    if(room_id < 0 or room_id > (int)rooms.size()) return false;
    room_info & room = rooms[room_id];
    for(int i = 0; i < n_connected_rooms; i++)
      room.connected_rooms.push_back( connected_rooms_ids[i] );
    return true;
  }

  void PrintRooms(){
    printf( "total %d rooms\n", (int) rooms.size() );
    for(unsigned int i = 0; i < rooms.size(); i++){
      printf("room %d, connections:", i);
      room_info & room = rooms[i];
      if( not room.connected_rooms.size() )
        printf("no connections !!!");
      for(unsigned int j = 0; j < room.connected_rooms.size(); j++){
        printf("%d ", room.connected_rooms[j]);
        if( room.connected_rooms[j] >= (int)rooms.size() )
          printf("<- not in rooms !!!");
      }
      printf("\n");
    }
  }


  ObjectAI * AddObject(const int & grid_type, const int & value, 
      int *grid_value_,
      int *dir_local_max_x_, int *dir_local_max_y_, int *local_max_,
      int *dir_local_min_x_, int *dir_local_min_y_, int *local_min_,
      int *dist_local_max_x_, int *dist_local_max_y_, int *dist_local_min_x_, int *dist_local_min_y_){
    while(true){
      std::map<unsigned int, ObjectAI*>::iterator it = objects.find( id_counter );
      if( it == objects.end() ) break;
      id_counter++;
    }

    ObjectAI * obj = new ObjectAI(grid_type, value,
        grid_value_,
        dir_local_max_x_, dir_local_max_y_, local_max_,
        dir_local_min_x_, dir_local_min_y_, local_min_,
        dist_local_max_x_, dist_local_max_y_, dist_local_min_x_, dist_local_min_y_);

    objects[id_counter] = obj;
    obj->id = id_counter;
    id_counter++;
    return obj;
  }

  void RemoveObject(ObjectAI * obj){
    const int & id = obj->id;
    std::map<unsigned int, ObjectAI*>::iterator it = objects.find( id );
    if( it == objects.end() ){
      printf("pmAI.RemoveObject(): can't remove object with id %d", id);
      return;
    }
    delete it->second;
    objects.erase( it );
  }

  // find path between rooms
  double RoomsEmpiricDistance(const room_info & room_1, const room_info & room_2){
    return std::abs(room_1.center_x - room_2.center_x) + std::abs(room_1.center_y - room_2.center_y);
  }

  void RoomsAstar(const room_info & room_start, const room_info & room_end, std::vector<int> & answer_path){
    if(room_start.id == room_end.id){
      answer_path.push_back( room_start.id );
      return;
    }

    // possible directions { room_id : [father id, empiric distance to end, distance from start] }
    std::map<int, room_astar_member*> open_list; 
    std::map<int, room_astar_member*> close_list; 
    room_astar_member * it_end;
    open_list[ room_start.id ] = new room_astar_member( NULL, -1, RoomsEmpiricDistance(room_start, room_end), 0 );

    while( true ){
      if( not open_list.size() ) goto exit_mark;
      std::map<int, room_astar_member*>::iterator it_min = open_list.begin();
      float minimal_path = it_min->second->distance_to_end + it_min->second->distance_from_start;
      for(std::map<int, room_astar_member*>::iterator it = open_list.begin(); it != open_list.end(); ++it){
        if( it->second->distance_to_end + it->second->distance_from_start >= minimal_path ) continue;
        minimal_path = it->second->distance_to_end + it->second->distance_from_start;
        it_min = it;
      }

      int checked_id = it_min->first;
      room_astar_member * checked_room = it_min->second;
      close_list[ it_min->first ] = checked_room;
      open_list.erase( it_min );

      const room_info & room = rooms[ checked_id ];
      for( unsigned int i = 0; i < room.connected_rooms.size(); i++ ){
        const int & id = room.connected_rooms[i];
        std::map<int, room_astar_member*>::iterator it_closed = close_list.find(id);
        if( it_closed != close_list.end() ) continue;
        const room_info & room_next = rooms[ id ];
        float distance_to_end = RoomsEmpiricDistance(room_next, room_end);
        std::map<int, room_astar_member*>::iterator it_prev = open_list.find(id);
        if( it_prev != open_list.end() ){
          if( it_prev->second->distance_from_start > checked_room->distance_from_start + 1 ){
            it_prev->second->father = checked_room;
            it_prev->second->distance_from_start = checked_room->distance_from_start + 1;
          }
        } else 
          open_list[ id ] = new room_astar_member( checked_room, id, distance_to_end, checked_room->distance_from_start + 1 );

        if(id == room_end.id) goto find_end_mark;
      }
    }

    find_end_mark : 
    it_end = open_list.find( room_end.id )->second;
    while( it_end->father != NULL ){
      answer_path.push_back( it_end->id );
      it_end = it_end->father;
      if( answer_path.size() > 50 ) break;
    }

    exit_mark :
    for(std::map<int, room_astar_member*>::iterator it = open_list.begin(); it != open_list.end(); it++)
      delete it->second;
    for(std::map<int, room_astar_member*>::iterator it = close_list.begin(); it != close_list.end(); it++)
      delete it->second;
  }

  PyObject* GetRoomPath(const unsigned int & start_room_id, const unsigned int & end_room_id){
    std::vector<int> answer_path; 
    if(start_room_id < rooms.size() and end_room_id < rooms.size()){
      const room_info & room_start = rooms[ start_room_id ];
      const room_info & room_end = rooms[ end_room_id ];
      // printf("path for %d %d \n", start_room_id, end_room_id);
      RoomsAstar( room_start, room_end, answer_path );
    }

    PyObject* answer = PyList_New( answer_path.size() );
    for(int i = answer_path.size()-1; i >= 0; --i)
      PyList_SetItem( answer, answer_path.size()-1 - i, PyInt_FromLong( answer_path[i] ) );
    return answer;
  }

  int tile_size;
  int max_astars_per_tick, done_astars_per_tick;
  int raytracer_bullets_steps;
  std::vector<room_info> rooms;

  unsigned int id_counter;
  std::map<unsigned int, ObjectAI*> objects;
};

  extern "C" {
    pmAI * pmAI_new(  ) {
      pmAI * answer = new pmAI( );
      printf("%d \n", answer);
      return answer;
    }

    // start position of the room (with minimum x,y value), for example top left corner, absolute value of pos and size (not in gamemap tiles!)
    int pmAI_AddRoom(pmAI * ai, int start_pos_x, int start_pos_y, int size_x, int size_y){
      return ai->AddRoom(start_pos_x, start_pos_y, size_x, size_y);
    }

    bool pmAI_SetRoomConnections(pmAI * ai, int room_id, int n_connected_rooms, int * connected_rooms_ids){
      return ai->SetRoomConnections(room_id, n_connected_rooms, connected_rooms_ids);
    }

    void pmAI_PrintRooms(pmAI * ai){
      ai->PrintRooms();
    }

    PyObject* pmAI_GetRoomPath(pmAI * ai, int start_room_id, int end_room_id){
      return ai->GetRoomPath(start_room_id, end_room_id);
    }

    ObjectAI * pmAI_AddObject(pmAI * ai, int grid_type, int value, 
      int *grid_value,
      int *dir_local_max_x_, int *dir_local_max_y_, int *local_max_,
      int *dir_local_min_x_, int *dir_local_min_y_, int *local_min_,
      int *dist_local_max_x_, int *dist_local_max_y_, int *dist_local_min_x_, int *dist_local_min_y_){
      return ai->AddObject(grid_type, value, grid_value, 
        dir_local_max_x_, dir_local_max_y_, local_max_,
        dir_local_min_x_, dir_local_min_y_, local_min_,
        dist_local_max_x_, dist_local_max_y_, dist_local_min_x_, dist_local_min_y_);
    }

    void pmAI_RemoveObject(pmAI * ai, ObjectAI * obj){
      return ai->RemoveObject(obj);
    }

    PyObject* pmAI_GetRoomGrid(pmAI * ai, unsigned int room_id, unsigned int type){
      PyObject* answer = PyList_New( 5 );
      if( room_id >= ai->rooms.size() ) return answer;
      if( type >= GRID_DEAPTH ) return answer;
      const room_info & room = ai->rooms[ room_id ];
      PyObject* grid_list = PyList_New( room.grid_size_x * room.grid_size_y );
      for( int x = 0; x < room.grid_size_x; x++ )
        for( int y = 0; y < room.grid_size_y; y++ )
          PyList_SetItem( grid_list, x * room.grid_size_y + y, PyInt_FromLong(room.grid[x][y][type]) );

      PyList_SetItem( answer, 0, PyInt_FromLong(room.grid_size_x) );
      PyList_SetItem( answer, 1, PyInt_FromLong(room.grid_size_y) );
      PyList_SetItem( answer, 2, PyInt_FromLong(room.maximums[type] ) );
      PyList_SetItem( answer, 3, PyInt_FromLong(room.minimums[type] ) );
      PyList_SetItem( answer, 4, grid_list );
      return answer;
    }

    void pmAI_Tick(pmAI * ai, int N_bullets, int * bullet_positions, float * bullet_speeds, int * bullet_rooms){
      ai->Tick(N_bullets, bullet_positions, bullet_speeds, bullet_rooms);
    }

    void pmAI_UpdateObjects(int N_objects, ObjectAI ** objects, int * object_positions, int * object_rooms){
      // update all objects at once
      for(int i = 0; i < N_objects; i++){
        printf("pmAI_UpdateObjects %d %d %d %d \n", N_objects, object_positions[2*i], object_positions[2*i+1], object_rooms[i]);
        objects[i]->Update( object_positions[2*i], object_positions[2*i+1], object_rooms[i] );
      }
    }
  }

  extern "C" int check(){ return 0; }

#endif



