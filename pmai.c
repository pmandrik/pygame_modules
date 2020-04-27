
#ifndef pmai
#define pmai 1

// g++ -Wall -Wfatal-errors -Wno-long-long -I/usr/include/python2.7 -lpython2.7  -Wextra -O -ansi -pedantic -shared -o pmai.so -fPIC pmai.c

#include <Python.h>
#include <vector>
#include <map>
#include <cmath>

#include "pmphysics.c"
#include "PMANDRIK_LIBRARY/pmlib_path_search.hh"

// #define DEBUG_CODE 1 
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

  // structute to be used for construction of the graph within rooms
  struct room_graph_member{
    room_graph_member(int start_x_, int start_y_, int end_x_, int end_y_){
      start_x = start_x_;
      start_y = start_y_;
      end_x = end_x_;
      end_y = end_y_;
      center_x = (start_x + end_x) / 2;
      center_y = (start_y + end_y) / 2;
    }
    
    room_graph_member(){}
    
    int GetNumberOfConnections(void) const {
      return connected_graphs.size();
    };
      
    int GetConnectionAt(const int & index) const {
      return connected_graphs.at(index);
    };
    
    float EmpiricDistance(const room_graph_member & other) const {
      int answer = pow(abs(other.center_x - center_x), 2) + pow(abs(other.center_y - center_y), 2);
      return answer + rand() % 100;
    };
    
    float RealDistance(const room_graph_member & other) const {
      return pow(abs(other.center_x - center_x), 2) + pow(abs(other.center_y - center_y), 2);
    }
    
    void Print() const {
      printf("room_graph_member x,y,w,h = %d,%d,%d,%d n_connects = %d\n", start_x, start_y, end_x-start_x, end_y-start_y, connected_graphs.size());
    }
    
    void SetConnectedEdges(const room_graph_member & other, int & x, int & y) const {
      if( start_x == other.end_x+1   ) x = start_x;
      if( end_x   == other.start_x+1 ) x = end_x;
      if( start_y == other.end_y+1   ) y = start_y;
      if( end_y   == other.start_y+1 ) y = end_y;
    }
    
    int start_x, start_y, end_x, end_y, center_x, center_y;
    std::vector<int> connected_graphs;
  };

// structute to be used to store info about rooms
class room_info  {
  public:
  room_info(const int & px, const int & py, const int & sx, const int & sy, pmTiledMap * map){
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
    printf( "pmai.c:room_info() ...\n");

    // grid massive to store information about objects, bullets etc
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
    
    // Create room path graph ...
    tstart_x = px / map->tsize;
    tstart_y = py / map->tsize;
    tend_x   = (px + sx) / map->tsize;
    tend_y   = (py + sy) / map->tsize;
    CreateGraph( map );
    
    printf( "pmai.c:room_info() ... ok\n");
  }

  int tstart_x, tstart_y, tend_x, tend_y;
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
  
  bool CheckIfPointInRoom(const int & x, const int & y) const {
    if( x < start_x or x >= start_x + size_x) return false;
    if( y < start_y or y >= start_y + size_y) return false;
    return true;
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
  
  // similare to update ...
  void AddLineToGraphs(const int & x, const int & ys, const int & ye){
    for(unsigned int i = 0; i < path_graphs.size(); i++){
      room_graph_member & gm = path_graphs.at(i);
      if( x-1 != gm.end_x ) continue; // we are only going to add new lines from left to right
      if(gm.start_y != ys or gm.end_y != ye) continue;
      gm.end_x += 1;
      gm.center_x = (gm.start_x + gm.end_x) / 2;
      return;
    }
    path_graphs.push_back( room_graph_member(x, ys, x, ye) );
  }
  
  void CreateGraph( pmTiledMap * map ){
    path_graphs.clear();
    // go over lines and put connected empty spaces in to rects 
    DPRINT( printf("room_info.CreateGraph(): %d %d \n", tstart_x, tend_x) );
    for(int x = tstart_x; x < tend_x; x++){
      int rect_start = -1;
      for(int y = tstart_y; y < tend_y; y++){
        bool is_solid = not map->GetTypeSafeI(x, y, 0);
        if(not is_solid and rect_start == -1){
          rect_start = y;
          continue;
        }
        if(not is_solid) continue;
        if(rect_start == -1) continue;
        DPRINT( printf("!!! ========> add \n") );
        AddLineToGraphs(x, rect_start, y-1);
        rect_start = -1;
      }
      if(rect_start == -1) continue;
      DPRINT( printf("!!! ========> add \n") );
      AddLineToGraphs(x, rect_start, tend_y-1);
    }
    
    DPRINT( printf("before splitting %d \n", path_graphs.size()) );
    std::vector< room_graph_member > new_graphs;
    for(int i = path_graphs.size()-1; i >= 0; i--){
      room_graph_member & gm = path_graphs.at(i);
      bool is_solid_prev_l = not map->GetTypeSafeI(gm.start_x-1, gm.start_y, 0);
      bool is_solid_prev_r = not map->GetTypeSafeI(  gm.end_x+1, gm.start_y, 0);
      for(int y = gm.start_y+1; y <= gm.end_y; y++){
        bool is_solid_l = not map->GetTypeSafeI(gm.start_x-1, y, 0);
        bool is_solid_r = not map->GetTypeSafeI(  gm.end_x+1, y, 0);
        
        if(is_solid_prev_l != is_solid_l or is_solid_prev_r != is_solid_r){
          is_solid_prev_l = is_solid_l;
          is_solid_prev_r = is_solid_r;
          
          new_graphs.push_back( room_graph_member(gm.start_x, gm.start_y, gm.end_x, y-1) );
          
          gm.start_y   = y;
          gm.center_y  = (gm.start_y + gm.end_y) / 2;
          DPRINT( printf("after splitting %d \n", path_graphs.size()) );
        }
      }
    }
    for(int i = new_graphs.size()-1; i >= 0; i--)
      path_graphs.push_back( new_graphs[i] );
    
    DPRINT( printf("after splitting %d \n", path_graphs.size()) );
    
    DPRINT( printf("room_info.CreateGraph(): make connections\n") );
    for(unsigned int i = 0; i < path_graphs.size(); i++){
      room_graph_member & gm_i = path_graphs.at(i);
      for(unsigned int j = i+1; j < path_graphs.size(); j++){
        room_graph_member & gm_j = path_graphs.at(j);
        if( gm_i.start_x-1 == gm_j.end_x or gm_i.end_x+1 == gm_j.start_x){
          if( gm_i.start_y > gm_j.end_y   ) continue;
          if( gm_i.end_y   < gm_j.start_y ) continue;
          gm_i.connected_graphs.push_back( j );
          gm_j.connected_graphs.push_back( i );
        }
        
        if( gm_i.start_y-1 == gm_j.end_y or gm_i.end_y+1 == gm_j.start_y){
          if( gm_i.start_x > gm_j.end_x   ) continue;
          if( gm_i.end_x   < gm_j.start_x ) continue;
          gm_i.connected_graphs.push_back( j );
          gm_j.connected_graphs.push_back( i );
        }
      }
    }
    DPRINT( printf("room_info.CreateGraph(): done ...\n") );
    if(true){
      for(unsigned int i = 0; i < path_graphs.size(); i++){
        const room_graph_member & gm_i = path_graphs.at(i);
        gm_i.Print();
      }
    }
  }
  
  int GetGraph(const int & pos_x, const int & pos_y) const {
    DPRINT( std::cout << "room_info.GetGraph(): " << path_graphs.size() << std::endl );
    for(unsigned int i = 0; i < path_graphs.size(); i++){
      const room_graph_member & gm_i = path_graphs.at(i);
      if(pos_x < gm_i.start_x or pos_x > gm_i.end_x) continue;
      if(pos_y < gm_i.start_y or pos_y > gm_i.end_y) continue;
      return i;
    }
    return -1;
  }
  
  std::vector< room_graph_member > path_graphs;
  
  // functions used to path search between rooms
  int GetNumberOfConnections() const {
    return connected_rooms.size();
  }
  
  int GetConnectionAt(const int & index) const {
    return connected_rooms.at( index );
  }
  
  float EmpiricDistance(const room_info & other) const {
    return std::abs(center_x - other.center_x) + std::abs(center_y - other.center_y);
  }
  
  float RealDistance(const room_info & other) const {
    return std::abs(center_x - other.center_x) + std::abs(center_y - other.center_y);
  }
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

    room_id = -1;
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
    pmAI( pmTiledMap * map ){
      printf("pmai.pmAI() ... ");
      max_astars_per_tick = 5;
      id_counter = 0;
      game_map = map;
      printf("ok\n");
      raytracer_bullets_steps = 5;

      DPRINT( std::cout << "QWERTY + " << rooms.size() << std::endl );
    }
    
    pmTiledMap * game_map;

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
        room_info & room = rooms.at( object->room_id ); // FIXME

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
    rooms.push_back( room_info(px, py, sx, sy, this->game_map) );
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

  // find path between rooms =====================================================
  PyObject* GetRoomPath(const unsigned int & start_room_id, const unsigned int & end_room_id){
    std::vector<int> answer_path; 
    if(start_room_id < rooms.size() and end_room_id < rooms.size()){
      // printf("path for %d %d \n", start_room_id, end_room_id);
      pm::Astar( this->rooms, start_room_id, end_room_id,  answer_path );
    }

    PyObject* answer = PyList_New( answer_path.size() );
    for(int i = answer_path.size()-1; i >= 0; --i)
      PyList_SetItem( answer, answer_path.size()-1 - i, PyInt_FromLong( answer_path[i] ) );
    return answer;
  }

  // find path within room =====================================================
  void ConstructInRoomPath(const room_info & room, std::vector< std::pair<int, int> > & answer, int start_position_x, int start_position_y, int end_position_x, int end_position_y){
    // check if points is within the given room
    if(not room.CheckIfPointInRoom(start_position_x, start_position_y)) return;
    if(not room.CheckIfPointInRoom(end_position_x, end_position_y))     return;
    
    // check if points are not in solid tiles ???
    start_position_x /= game_map->tsize;
    start_position_y /= game_map->tsize;
    end_position_x /= game_map->tsize;
    end_position_y /= game_map->tsize;
    if( not game_map->GetTypeSafeI(start_position_x, start_position_y, 0) ) return;
    if( not game_map->GetTypeSafeI(end_position_x, end_position_y, 0) ) return;
      
    // find path using in-room graphs
    int start_graph_index = room.GetGraph(start_position_x, start_position_y);
    int end_graph_index   = room.GetGraph(end_position_x, end_position_y);
    if(start_graph_index == -1 or end_graph_index == -1)
      return;
    
    DPRINT( std::cout << "pmAI.ConstructInRoomPath() ... going to call A* for " << start_graph_index << " " << end_graph_index << "indexes" << std::endl );
    std::vector<int> graph_path;
    pm::Astar(room.path_graphs, start_graph_index, end_graph_index, graph_path);
    
    if( graph_path.size() >= 2)
      graph_path.push_back( start_graph_index );
    
    if(false){ // use room centers for debug
      for(int i = graph_path.size()-1; i >= 0; i--){
        const room_graph_member & rgm = room.path_graphs.at( graph_path[i] );
        answer.push_back( std::make_pair<int, int>(rgm.center_x, rgm.center_y) );
      }
      answer.push_back( std::make_pair<int, int>(end_position_x, end_position_y) );
      return;
    }
      
    // construct real path TODO
    int px = start_position_x;
    int py = start_position_y;
    
    for(int i = graph_path.size()-1; i > 0; i--){
      const room_graph_member & rgm = room.path_graphs.at( graph_path[i] );
      if( i == 0 ){
        answer.push_back( std::make_pair<int, int>(rgm.center_x, rgm.center_y) );
        continue;
      }
      const room_graph_member & rgm_next = room.path_graphs.at( graph_path[i-1] );
      
      bool change_x = false;
      if( rgm.start_x == rgm_next.end_x+1 ){
        px = rgm.start_x;
        change_x = true;
      } else if(rgm.end_x == rgm_next.start_x-1) {
        px = rgm.end_x;
        change_x = true;
      } else if(rgm.start_y == rgm_next.end_y+1) {
        py = rgm.start_y;
      } else if(rgm.end_y == rgm_next.start_y-1) {
        py = rgm.end_y;
      }
      
      
      if(not change_x){
        int start = std::max( rgm.start_x, rgm_next.start_x );
        int end   = std::min( rgm.end_x, rgm_next.end_x );
        px = std::max(px, std::min(start+2, end));
        px = std::min(px, std::max(end-2, start));
      } else {
        int start = std::max( rgm.start_y, rgm_next.start_y );
        int end   = std::min( rgm.end_y, rgm_next.end_y );
        py = std::max(py, std::min(start+2, end));
        py = std::min(py, std::max(end-2, start));
        printf(">>> %d %d %d", py, start, end);
      }
      answer.push_back( std::make_pair<int, int>(px, py) );
    }
    answer.push_back( std::make_pair<int, int>(end_position_x, end_position_y) );
  }
  
  PyObject* GetInRoomPath(const unsigned int & room_id, const int & start_position_x, const int & start_position_y, const int & end_position_x, const int & end_position_y){
    DPRINT( std::cout << "pmAI.GetInRoomPath() ... " << room_id << " " << start_position_x << " " << start_position_y << " " << end_position_x << " " << end_position_x << std::endl );

    std::vector< std::pair<int, int> > answer_path; 
    if( room_id < rooms.size() ){
      const room_info & room = rooms[ room_id ];
      ConstructInRoomPath( room, answer_path, start_position_x, start_position_y, end_position_x, end_position_y );
    }
    
    DPRINT( std::cout << "pmAI.GetInRoomPath() : creating answer PyObject" << std::endl );
    PyObject* answer = PyList_New( answer_path.size() );
    for(int i = answer_path.size()-1; i >= 0; --i){
      DPRINT( std::cout << i << " !!1 1" << std::endl );
      PyObject* point = PyList_New( 2 );
      DPRINT( std::cout << i << " !!1 2" << std::endl );
      PyList_SetItem(point, 0, PyInt_FromLong( game_map->tsize/2 + game_map->tsize * answer_path[i].first ) );
      DPRINT( std::cout << i << " !!1 3" << std::endl );
      PyList_SetItem(point, 1, PyInt_FromLong( game_map->tsize/2 + game_map->tsize * answer_path[i].second ) );
      DPRINT( std::cout << i << " !!1 4" << std::endl );
      PyList_SetItem( answer, answer_path.size()-1-i, point );
    }
    DPRINT( std::cout << "pmAI.GetInRoomPath() ... return" << std::endl );
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
    void * pmAI_new( pmTiledMap * game_map ) {
      DPRINT( std::cout << "pmAI_new()" << std::endl );
      pmAI * answer = new pmAI( game_map );
      return answer;
    }

    // start position of the room (with minimum x,y value), for example top left corner, absolute value of pos and size (not in gamemap tiles!)
    int pmAI_AddRoom(void * ai, int start_pos_x, int start_pos_y, int size_x, int size_y){
      DPRINT( std::cout << "pmAI_AddRoom()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      int answer = ai_->AddRoom(start_pos_x, start_pos_y, size_x, size_y);
      DPRINT( std::cout << "pmAI_AddRoom() " << answer << std::endl );
      return answer;
    }

    bool pmAI_SetRoomConnections(void * ai, int room_id, int n_connected_rooms, int * connected_rooms_ids){
      DPRINT( std::cout << "pmAI_SetRoomConnections()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      return ai_->SetRoomConnections(room_id, n_connected_rooms, connected_rooms_ids);
    }

    void pmAI_PrintRooms(void * ai){
      DPRINT( std::cout << "pmAI_PrintRooms()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      ai_->PrintRooms();
    }

    PyObject* pmAI_GetRoomPath(void * ai, int start_room_id, int end_room_id){
      DPRINT( std::cout << "pmAI_GetRoomPath()" << std::endl );
      PyGILState_STATE gstate = PyGILState_Ensure();
      pmAI * ai_ = static_cast<pmAI*>( ai );
      PyObject* answer = ai_->GetRoomPath(start_room_id, end_room_id);
      PyGILState_Release(gstate);
      return answer;
    }
    
    PyObject* pmAI_GetInRoomGraphs(void * ai, int room_id){
      PyGILState_STATE gstate = PyGILState_Ensure();
      pmAI * ai_ = static_cast<pmAI*>( ai );
      const room_info & room = ai_->rooms[ room_id ];
      const std::vector< room_graph_member > & path_graphs = room.path_graphs;
      
      PyObject* answer = PyList_New( path_graphs.size() );
      pmTiledMap * game_map = ai_->game_map;
      for(unsigned int i = 0; i < path_graphs.size(); i++){
        const room_graph_member & gm_i = path_graphs.at(i);
        
        PyObject* item = PyList_New( 4 );
        PyList_SetItem(item, 0, PyInt_FromLong(game_map->tsize/2 +game_map->tsize * gm_i.start_x) );
        PyList_SetItem(item, 1, PyInt_FromLong(game_map->tsize/2 +game_map->tsize * gm_i.start_y) );
        PyList_SetItem(item, 2, PyInt_FromLong(game_map->tsize/2 +game_map->tsize * gm_i.end_x) );
        PyList_SetItem(item, 3, PyInt_FromLong(game_map->tsize/2 +game_map->tsize * gm_i.end_y) );
        
        PyList_SetItem( answer, i, item );
      }
      PyGILState_Release(gstate);
      return answer;
    }
    
    PyObject* pmAI_GetInRoomPath(void * ai, int room_id, int start_position_x, int start_position_y, int end_position_x, int end_position_y){
      DPRINT( std::cout << "pmAI_GetInRoomPath()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      PyGILState_STATE gstate = PyGILState_Ensure();
      PyObject* answer = ai_->GetInRoomPath(room_id, start_position_x, start_position_y, end_position_x, end_position_y);
      PyGILState_Release(gstate);
      return answer;
    }
    
    void pmAI_CheckIfTargetsVisible(void * ai, int N_objects, int * obj_pos, int * targ_pos, bool * answers){
      DPRINT( std::cout << "pmAI_CheckIfTargetsVisible() ..." << N_objects << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      pmTiledMap * game_map = ai_->game_map;
      for(int i = 0; i < N_objects; i++){
        // DPRINT( std::cout << "pmAI_CheckIfTargetsVisible() ... process object " << i << " " << obj_pos[2*i] << " " << obj_pos[2*i+1] << " " << targ_pos[2*i] << " " <<  targ_pos[2*i+1]<< std::endl );
        answers[i] = game_map->CheckRayTracer( obj_pos[2*i], obj_pos[2*i+1], targ_pos[2*i], targ_pos[2*i+1] );
      }
      DPRINT( std::cout << "pmAI_CheckIfTargetsVisible() ... return" << std::endl );
    }
    
    void pmAI_RebuildRoomGraphs(void * ai, int n_rooms_ids, int * room_ids){
      pmAI * ai_ = static_cast<pmAI*>( ai );
      for(int i = 0; i < n_rooms_ids; i++){
        int room_id = room_ids[i];
        DPRINT( std::cout << "pmAI_RebuildRoomGraphs():" << room_id << std::endl );
        ai_->rooms.at( room_id ).CreateGraph( ai_->game_map );
      }
    }

    ObjectAI * pmAI_AddObject(void * ai, int grid_type, int value, 
      int *grid_value,
      int *dir_local_max_x_, int *dir_local_max_y_, int *local_max_,
      int *dir_local_min_x_, int *dir_local_min_y_, int *local_min_,
      int *dist_local_max_x_, int *dist_local_max_y_, int *dist_local_min_x_, int *dist_local_min_y_){
      DPRINT( std::cout << "pmAI_AddObject()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      return ai_->AddObject(grid_type, value, grid_value, 
        dir_local_max_x_, dir_local_max_y_, local_max_,
        dir_local_min_x_, dir_local_min_y_, local_min_,
        dist_local_max_x_, dist_local_max_y_, dist_local_min_x_, dist_local_min_y_);
    }

    void pmAI_RemoveObject(void * ai, ObjectAI * obj){
      DPRINT( std::cout << "pmAI_RemoveObject()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      return ai_->RemoveObject(obj);
    }

    PyObject* pmAI_GetRoomGrid(void * ai, unsigned int room_id, unsigned int type){
      DPRINT( std::cout << "pmAI_GetRoomGrid()" << std::endl );
      PyGILState_STATE gstate = PyGILState_Ensure();
      
      PyObject* answer = PyList_New( 5 );

      pmAI * ai_ = static_cast<pmAI*>( ai );
      if( room_id < ai_->rooms.size() and type < GRID_DEAPTH ){
        const room_info & room = ai_->rooms[ room_id ];

        PyObject* grid_list = PyList_New( room.grid_size_x * room.grid_size_y );
        for( int x = 0; x < room.grid_size_x; x++ )
          for( int y = 0; y < room.grid_size_y; y++ )
            PyList_SetItem( grid_list, x * room.grid_size_y + y, PyInt_FromLong(room.grid[x][y][type]) );

        PyList_SetItem( answer, 0, PyInt_FromLong(room.grid_size_x) );
        PyList_SetItem( answer, 1, PyInt_FromLong(room.grid_size_y) );
        PyList_SetItem( answer, 2, PyInt_FromLong(room.maximums[type] ) );
        PyList_SetItem( answer, 3, PyInt_FromLong(room.minimums[type] ) );
        PyList_SetItem( answer, 4, grid_list );
      }

      DPRINT( std::cout << "pmAI_GetRoomGrid() end " << std::endl );
      
      PyGILState_Release(gstate);
      return answer;
    }

    void pmAI_Tick(void * ai, int N_bullets, int * bullet_positions, float * bullet_speeds, int * bullet_rooms){
      DPRINT( std::cout << "pmAI_Tick()" << std::endl );
      pmAI * ai_ = static_cast<pmAI*>( ai );
      ai_->Tick(N_bullets, bullet_positions, bullet_speeds, bullet_rooms);
    }

    void pmAI_UpdateObjects(int N_objects, ObjectAI ** objects, int * object_positions, int * object_rooms){
      // update all objects at once
      for(int i = 0; i < N_objects; i++){
        DPRINT( printf("pmAI_UpdateObjects %d %d %d %d \n", N_objects, object_positions[2*i], object_positions[2*i+1], object_rooms[i]) );
        objects[i]->Update( object_positions[2*i], object_positions[2*i+1], object_rooms[i] );
      }
    }
  }

  extern "C" int check_pmai(){ return 0; }

#endif



