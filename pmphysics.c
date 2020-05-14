///////////////////////////////////////////////////////////////////////////////
//  Author      :     P.S. Mandrik, IHEP
//  Date        :     19/03/20
//  Last Update :     19/03/20
//  Version     :     1.0
//  physic system for external usage in Python 2.7
///////////////////////////////////////////////////////////////////////////////

#ifndef pmphysics
#define pmphysics 1

// g++ -Wfatal-errors -Wall -Wno-long-long -I/usr/include/python2.7 -lpython2.7  -Wextra -O -ansi -pedantic -shared -o pmphysics.so -fPIC pmphysics.c

#include <Python.h>
#include <vector>
#include <map>
#include <cmath>

#include "PMANDRIK_LIBRARY/pmlib_v2d.hh" // pm::v2
#include "PMANDRIK_LIBRARY/pmlib_const.hh" // pm::PI_180 = 3.141 / 180.

// #define DEBUG_CODE 1 
#ifdef DEBUG_CODE 
#include <iostream>
#define DPRINT(x) (x)
#else 
#define DPRINT(x) do{}while(0)
#endif

  // class to interact with tiled map data =====================
  class pmTiledMap{
    public:
    pmTiledMap( const int & map_size_x, const int & map_size_y, const int & map_size_types, const int & tile_size, int * map_data ){
      msize_x = map_size_x;
      msize_y = map_size_y;
      msize_types = map_size_types;
      msize_types_x_msize_y = msize_y * msize_types;
      tsize = tile_size;
      mdata = map_data;
    }
    
    int msize_x, msize_y, msize_types, msize_types_x_msize_y;
    int tsize, bullet_grid_size;
    int * mdata;
    
    void PrintMap(const int & layer) const {
      for(int y = 0; y < msize_y; y++){
        for(int x = 0; x < msize_x; x++)
          printf("%d", mdata[msize_types_x_msize_y*x + msize_types*y + layer] );
        printf("\n");
      }
    }

    int GetMapIndexSafe( const float & px, const float & py ) const {
      int ipx = int(px) / tsize;
      int ipy = int(py) / tsize;
      if(ipx < 0 or ipx > msize_x or ipy < 0 or ipy > msize_y){
        printf("pmPhysic.GetMapIndexSafe(): invalid request %f %f for map %i %i %i \n", px, py, msize_x, msize_y, tsize);
        return 0;
      }
      return msize_types_x_msize_y*ipx + msize_types*ipy;
    }
    
    int GetTypeSafeI( const int & ipx, const int & ipy, const int & type_index) const {
      if(ipx < 0 or ipx > msize_x or ipy < 0 or ipy > msize_y or type_index < 0 or type_index > msize_types){
        printf("pmPhysic.GetTypeSafeI(): invalid request %d %d %d \n", ipx, ipy, type_index);
        return 0;
      }
      return mdata[ msize_types_x_msize_y*ipx + msize_types*ipy + type_index ];
    }

    int GetTypeSafe( const float & px, const float & py, const int & type_index) const {
      int ipx = int(px) / tsize;
      int ipy = int(py) / tsize;
      return GetTypeSafeI(ipx, ipy, type_index);
    }

    void SetMapTypeSafe(const int & x, const int & y, const float * map_data){
      if(x < 0 or y < 0 or x > msize_x or y > msize_y) return;
      memcpy(mdata + msize_types_x_msize_y*x + msize_types*y, map_data, msize_types * sizeof(int));
    }
    
    bool CheckRayTracer(float pos_x, float pos_y, float targ_pos_x, float targ_pos_y){
      int room_id      = GetTypeSafe(pos_x, pos_y, 1);
      if( room_id != GetTypeSafe(targ_pos_x, targ_pos_y, 1) ) return false;
      float dx = targ_pos_x - pos_x;
      float dy = targ_pos_y - pos_y;
      float L = sqrt(dx*dx+dy*dy);
      float step_dist = 0.9*tsize;
      dx = step_dist / L * dx;
      dy = step_dist / L * dy;
      int max_steps = L / step_dist;
      if( max_steps < 1 ) return true; 
      for(int i = 0; i < max_steps; i++){
        pos_x += dx;
        pos_y += dy;
        if( room_id != GetTypeSafe(pos_x, pos_y, 1) ) return false;
        if( not GetTypeSafe(pos_x, pos_y, 0) ) return false;
      }
      return true;
    }
  };

  // class to calculate objects, bullets and map interations =========
  class pmPhysic{
    public:
    pmPhysic( pmTiledMap * map ){
      printf("pmphysics.pmPhysic() ... ");
      game_map         = map;
      mdata            = map->mdata;
      bullet_grid_size = map->tsize*5;
      msize_types      = map->msize_types;
      printf("ok\n");
    }
    
    pmTiledMap * game_map;
    int msize_types;
    int * mdata;

    bool CheckLinesIntersection(const pm::v2 & s1, const pm::v2 & e1, const pm::v2 & s2, const pm::v2 & e2){
      float dx1 = e1.x - s1.x;
      float dy1 = e1.y - s1.y;
      float dx2 = e2.x - s2.x;
      float dy2 = e2.y - s2.y;
      float scalar = dy1 * dx2  - dx1 * dy2;
      if( scalar < 0.001 ) return false;
      float alpha = ((s1.x - s2.x) * dy1 - (s1.y - s2.y) * dx1)/scalar;
      float betta = ((s1.x - s2.x) * dy2 - (s1.y - s2.y) * dx2)/scalar;
      if( alpha < 0 or alpha > 1 ) return false;
      if( betta < 0 or betta > 1 ) return false;
      return true;
    }

    bool CheckIfPointInRect(const pm::v2 & c, const pm::v2 & sv1, const pm::v2 & sv2, pm::v2 point){
      // check 4 points for rect solving equation v2(point_x - pos_x, point_y - pos_y) = a * (sv1) + b * (sv2), |a| < 1, |b| < 1 , |a| + |b| < 1
      // |y * x1 - x * y1| < | y2 * x1 - x2 * y1 |
      // |y * x2 - x * y2| < | y2 * x1 - x2 * y1 |
      point.x -= c.x;
      point.y -= c.y;
      if(std::abs(point.y * sv1.x - point.x * sv1.y) + std::abs(point.y * sv2.x - point.x * sv2.y) > std::abs( sv2.y * sv1.x - sv2.x * sv1.y ) ) return false;
      //printf("%f %f \n", c.x, c.y);
      //printf("%f %f %f %f %f %f \n", point.y, sv1.x, sv1.y, point.x, sv2.x, sv2.y);
      //printf("%f %f \n", point.y * sv1.x - point.x * sv1.y, point.y * sv2.x - point.x * sv2.y);
      return true;
    }

    bool CheckRectCollisions(const pm::v2 & c1, const pm::v2 & sv11, const pm::v2 & sv12, const pm::v2 & c2, const pm::v2 & sv21, const pm::v2 & sv22){
      std::vector< pm::v2 > edges_1; 
      edges_1.push_back ( c1 + sv11 );
      edges_1.push_back ( c1 + sv12 ); 
      edges_1.push_back ( c1 - sv11 );
      edges_1.push_back ( c1 - sv12 );
      edges_1.push_back ( c1 + sv11 );
      std::vector< pm::v2 > edges_2; 
      edges_2.push_back ( c2 + sv21 );
      edges_2.push_back ( c2 + sv22 ); 
      edges_2.push_back ( c2 - sv21 );
      edges_2.push_back ( c2 - sv22 );
      edges_2.push_back ( c2 + sv21 );

      // check collisions using edges intersections
      for(int i = 0, i_max = edges_1.size() - 1; i < i_max; i++){
        pm::v2 & s1 = edges_1[i];
        pm::v2 & e1 = edges_1[i+1];
        for(int j = 0, j_max = edges_2.size() - 1; j < j_max; j++)
          if( CheckLinesIntersection( s1 , e1 , edges_2[j], edges_2[j+1] ) ) return true;
      }

      // check in addition if vertexes are in rect
      for(int i = 0, i_max = edges_1.size() - 1; i < i_max; i++){
        if( CheckIfPointInRect(c1, sv11, sv12, edges_2[i]) ) return true;
        if( CheckIfPointInRect(c2, sv21, sv22, edges_1[i]) ) return true;
      }
      return false;
    }

    void Tick(const int & N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_sizes_xy, float * object_angles, bool * object_x_solids,
              const int & N_bullets, float * bullet_positions, int * bullet_tiles, 
              std::vector< std::pair<int, int> > & interacted_object_x_object_pairs, std::vector< std::pair<int, int> > & interacted_bullet_x_object_pairs){
      // every Tick step we will care about:
      // - check if objects is going to interact with solid tiles
      //   => return the maximum available speed
      //   => return list of types of the tiles we interact with
      // - check if object is going to interact with another object
      //   => return ids of objects
      // - check bullets is about interation with solid tiles
      //   => return such information too
      // - check if object is going to interact with bullets
      //   => return list of [id_bullet, (id_objects, ... , )]
      int index, type;

      pm::v2 * size_vectors = new pm::v2 [2 * N_objects];
      pm::v2 * sv1, *sv2, *nsv1, *nsv2;

      // - check if objects is going to interact with solid tiles
      std::map< std::pair<int,int>, std::vector<int> > obj_grid;
      float pos_x, pos_y, speed_x, speed_y, npos_x = 0.f, npos_y = 0.f;
      float size_x, size_y, ccos, ssin;
      for(int i = 0, i_1 = 1, i_2=0, i_max = 2*N_objects; i < i_max; i+=2){
        pos_x = object_positions[ i   ];
        pos_y = object_positions[ i_1 ];

        speed_x = object_speeds[ i   ];
        speed_y = object_speeds[ i_1 ];

        // rotate objects
        size_x = object_sizes_xy[ i ]   ;
        size_y = object_sizes_xy[ i_1 ] ;
        ccos = cos( object_angles[i_2] * pm::PI_180 );
        ssin = sin( object_angles[i_2] * pm::PI_180 );

        size_vectors[ i   ].x =   size_x * ccos + size_y * ssin;
        size_vectors[ i   ].y = - size_x * ssin + size_y * ccos;
        size_vectors[ i_1 ].x =   size_x * ccos - size_y * ssin;
        size_vectors[ i_1 ].y = - size_x * ssin - size_y * ccos;

        sv1 = & size_vectors[ i   ];
        sv2 = & size_vectors[ i_1 ];

        npos_x = pos_x + speed_x;
        npos_y = pos_y + speed_y;

        // by default map is filled with 0 - solid tiles
        object_x_solids[ i_2 ] = false;
        if( not game_map->GetTypeSafe(npos_x, pos_y, 0) ){
          npos_x = pos_x;
          speed_x = 0;
          object_x_solids[ i_2 ] = true;
        }
        if( not game_map->GetTypeSafe(pos_x, npos_y, 0) ){
          npos_y = pos_y;
          speed_y = 0;
          object_x_solids[ i_2 ] = true;
        }
        if( not game_map->GetTypeSafe(npos_x, npos_y, 0) ){
          speed_x = 0;
          speed_y = 0;
          npos_x = pos_x;
          npos_y = pos_y;
          object_x_solids[ i_2 ] = true;
        }

        // => return the maximum available speed TODO
        object_positions[ i   ] = npos_x;
        object_positions[ i_1 ] = npos_y;
        object_speeds[ i   ] = speed_x;
        object_speeds[ i_1 ] = speed_y;

        // create a grid also
        //obj_grid[std::make_pair<int, int>(int(npos_x - sv1->x)/bullet_grid_size, int(npos_y - sv1->y)/bullet_grid_size)].push_back( i );
        //obj_grid[std::make_pair<int, int>(int(npos_x + sv1->x)/bullet_grid_size, int(npos_y + sv1->y)/bullet_grid_size)].push_back( i );
        //obj_grid[std::make_pair<int, int>(int(npos_x - sv2->x)/bullet_grid_size, int(npos_y - sv2->y)/bullet_grid_size)].push_back( i );
        //obj_grid[std::make_pair<int, int>(int(npos_x + sv2->x)/bullet_grid_size, int(npos_y + sv2->y)/bullet_grid_size)].push_back( i );
        //obj_grid[std::make_pair<int, int>(int(npos_x)/bullet_grid_size, int(npos_y)/bullet_grid_size)].push_back( i );

        float L = sqrt(size_x * size_x + size_y*size_y);
        for(int sx = 0; sx < L; sx += bullet_grid_size)
          for(int sy = 0; sy < L; sy += bullet_grid_size){
            obj_grid[std::make_pair<int, int>(int(sx + npos_x)/bullet_grid_size, int(sy + npos_y)/bullet_grid_size)].push_back( i );
            if(sx) obj_grid[std::make_pair<int, int>(int(npos_x - sx)/bullet_grid_size, int(sy + npos_y)/bullet_grid_size)].push_back( i );
            if(sy) obj_grid[std::make_pair<int, int>(int(sx + npos_x)/bullet_grid_size, int(npos_y - sy)/bullet_grid_size)].push_back( i );
            if(sx and sy) obj_grid[std::make_pair<int, int>(int(npos_x - sx)/bullet_grid_size, int(npos_y - sy)/bullet_grid_size)].push_back( i );
          }
        obj_grid[std::make_pair<int, int>(int(npos_x - L)/bullet_grid_size, int(npos_y - L)/bullet_grid_size)].push_back( i );
        obj_grid[std::make_pair<int, int>(int(npos_x + L)/bullet_grid_size, int(npos_y + L)/bullet_grid_size)].push_back( i );
        obj_grid[std::make_pair<int, int>(int(npos_x - L)/bullet_grid_size, int(npos_y - L)/bullet_grid_size)].push_back( i );
        obj_grid[std::make_pair<int, int>(int(npos_x + L)/bullet_grid_size, int(npos_y + L)/bullet_grid_size)].push_back( i );

        //   => return list of types of the tiles we interact with
        index = game_map->GetMapIndexSafe(npos_x, npos_y);
        for(type = 0; type < msize_types; type++)
          object_tiles[msize_types*i_2 + type] = mdata[index + type];

        i_1+=2;
        i_2++;
      }

      // - check if object is going to interact with another object
      float point_x, point_y;
      for(int i = 0, i_max = 2*N_objects; i < i_max; i+=2){
        pos_x = object_positions[ i   ];
        pos_y = object_positions[ i+1 ];

        sv1 = & size_vectors[ i   ];
        sv2 = & size_vectors[ i+1 ];

        for(int j = i+2; j < i_max; j+=2){
          npos_x = object_positions[ j   ];
          npos_y = object_positions[ j+1 ];
          nsv1 = & size_vectors[ j   ];
          nsv2 = & size_vectors[ j+1 ];

          if( not CheckRectCollisions( pm::v2(pos_x, pos_y), *sv1, *sv2, pm::v2(npos_x, npos_y), *nsv1, *nsv2) ) continue;
          //if( abs(pos_x - npos_x) > size_x + nsize_x or abs(pos_y - npos_y) > size_y + nsize_y ) continue;
          // => return ids of objects
          interacted_object_x_object_pairs.push_back( std::make_pair<int, int>(i/2, j/2) );
        }
      }

      // - check bullet is about interation with solid tiles
      // - check if object is going to interact with bullets
      for(int i = 0, i_2 = 0, i_max = 2*N_bullets; i < i_max; i+=2){
        pos_x = bullet_positions[ i   ];
        pos_y = bullet_positions[ i+1 ];

        //   => return list of types of the tiles we interact with
        index = game_map->GetMapIndexSafe(pos_x, pos_y);
        for(type = 0; type < msize_types; type++)
          bullet_tiles[msize_types*i_2 + type] = mdata[index + type];
         
        // hardcode solution for testing purpose
        /*
        printf("process bullet %d %f %f \n", i_2, pos_x, pos_y);
        for(int j = 0, j_2=-1, j_max = 2*N_objects; j < j_max; j+=2){
          j_2++;
          if( pow(pos_x - object_positions[ j   ], 2) + pow(pos_y - object_positions[ j+1 ], 2) > pow(object_r_sizes[j_2], 2) ) continue; 
          // interacted_bullet_x_object_pairs.push_back( std::make_pair<int, int>(i_2, j_2) );
          printf("I. add %d %d pairs ... \n", i_2, j_2);
        }
        */
        
        std::pair<int,int> bkey = std::make_pair<int, int>(int(pos_x)/bullet_grid_size, int(pos_y)/bullet_grid_size);
        if( obj_grid.find(bkey) != obj_grid.end() ){
          const std::vector<int> & ids = obj_grid.find(bkey)->second;
          for(unsigned int id_index = 0; id_index != ids.size(); id_index++){
            int obj_id = ids[ id_index ];

            // check 4 points for rect solving equation v2(point_x - pos_x, point_y - pos_y) = a * (sv1) + b * (sv2), |a| < 1, |b| < 1, |a| + |b| < 1
            // |y * x1 - x * y1| < | y2 * x1 - x2 * y1 |
            // |y * x2 - x * y2| < | y2 * x1 - x2 * y1 |
            sv1 = & size_vectors[ obj_id   ];
            sv2 = & size_vectors[ obj_id+1 ];
            // printf("sv1, sv2 %f %f %f %f \n", sv1->x, sv1->y, sv2->x, sv2->y);
            point_x = pos_x - object_positions[obj_id]  ;
            point_y = pos_y - object_positions[obj_id+1];
            // printf("point_x, point_y %f %f\n", point_x, point_y);
            // printf("A, B %f %f\n", abs(point_y * sv1->x - point_x * sv1->y) / abs( sv2->y * sv1->x - sv2->x * sv1->y ), abs(point_y * sv2->x - point_x * sv2->y) / abs( sv2->y * sv1->x - sv2->x * sv1->y ));
            if(abs(point_y * sv1->x - point_x * sv1->y) + abs(point_y * sv2->x - point_x * sv2->y) > abs( sv2->y * sv1->x - sv2->x * sv1->y )) continue;

            // if( abs(pos_x - object_positions[obj_id]) > object_sizes_xy[obj_id] or abs(pos_y - object_positions[obj_id+1]) > object_sizes_xy[obj_id+1] ) continue; // bullet is a point
            // => return ids of objects
            interacted_bullet_x_object_pairs.push_back( std::make_pair<int, int>(i_2, obj_id/2) );
          }
        }

        i_2++;
      }

      // - check if objects is going to interact with lines ... (such as laser, lighting traps etc)
      // TODO

      delete [] size_vectors;
    }

    PyObject* FillPyListFromPairVector(const std::vector< std::pair<int, int> > & vec){
      PyObject* answer = PyList_New( 2*vec.size() );
      for(int i = 0, imax = vec.size(), list_index=0; i<imax; ++i){
        PyList_SetItem( answer, list_index,   PyInt_FromLong( vec[i].first  ) );
        PyList_SetItem( answer, list_index+1, PyInt_FromLong( vec[i].second ) );
        list_index+=2;
      }
      return answer;
    }

    int bullet_grid_size;
  };

  extern "C" {
    void * pmTiledMap_new( int map_size_x, int map_size_y, int map_size_types, int tile_size, int * map_data ) {
      DPRINT( std::cout << "pmTiledMap_new()" << std::endl );
      pmTiledMap * answer = new pmTiledMap( map_size_x, map_size_y, map_size_types, tile_size, map_data );
      DPRINT( std::cout << " ... " << answer->tsize << std::endl );
      return answer;
    }
    
    void pmTiledMap_PrintMap(void * game_map, int layer){
      DPRINT( std::cout << "pmPhysic_PrintMap()" << std::endl );
      pmTiledMap * game_map_ = static_cast<pmTiledMap*>(game_map);
      game_map_->PrintMap(layer);
    }
    
    void * pmPhysic_new( void * game_map ) {
      DPRINT( std::cout << "pmPhysic_new()" << std::endl );
      pmTiledMap * game_map_ = static_cast<pmTiledMap*>(game_map);
      pmPhysic * answer = new pmPhysic( game_map_ );
      DPRINT( std::cout << " ... " << answer->tsize << std::endl );
      return answer;
    }

    void pmPhysic_del(void * physic){
      DPRINT( std::cout << "pmPhysic_del()" << std::endl );
      pmPhysic* physic_ = static_cast<pmPhysic*>(physic);
      delete physic_;
    }

    PyObject* pmPhysic_Tick(void * physic, int N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_sizes_xy, float * object_angles, bool * object_x_solids, int N_bullets, float * bullet_positions, int * bullet_tiles) {
      DPRINT( std::cout << "pmPhysic_PyTick()" << std::endl );
      pmPhysic* physic_ = static_cast<pmPhysic*>(physic);

      std::vector< std::pair<int, int> > interacted_object_x_object_pairs;
      std::vector< std::pair<int, int> > interacted_bullet_x_object_pairs;
      physic_->Tick( N_objects, object_positions, object_speeds, object_tiles, object_sizes_xy, object_angles, object_x_solids, N_bullets, bullet_positions, bullet_tiles, interacted_object_x_object_pairs, interacted_bullet_x_object_pairs);

      PyGILState_STATE gstate = PyGILState_Ensure();
      PyObject* answer = PyList_New(2);
      PyObject* interacted_object_x_object_pairs_PyList = physic_->FillPyListFromPairVector( interacted_object_x_object_pairs );
      PyObject* interacted_bullet_x_object_pairs_PyList = physic_->FillPyListFromPairVector( interacted_bullet_x_object_pairs );

      PyList_SetItem( answer, 0, interacted_object_x_object_pairs_PyList);
      PyList_SetItem( answer, 1, interacted_bullet_x_object_pairs_PyList);
      PyGILState_Release(gstate);
      return answer;
    }
  }

  extern "C" int check_pmphysic(){ return 0; }

#endif



