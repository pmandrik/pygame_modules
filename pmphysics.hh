///////////////////////////////////////////////////////////////////////////////
//  Author      :     P.S. Mandrik, IHEP
//  Date        :     19/03/20
//  Last Update :     19/03/20
//  Version     :     1.0
//  physic system for external usage in Python 2.7
///////////////////////////////////////////////////////////////////////////////

#ifndef pmphysics
#define pmphysics 1

// gcc -std=c99 -shared -o pmphysics.so -fPIC pmphysics.hh
// g++ -Wall -Wextra -O -ansi -pedantic -shared -o pmphysics.so -fPIC pmphysics.hh
// g++ -Wall -Wno-long-long -I/usr/include/python2.7 -lpython2.7  -Wextra -O -ansi -pedantic -shared -o pmphysics.so -fPIC pmphysics.hh

/*


#include <Python.h>
#include <vector>
#include <map>

  class pmPhysic{
    public:
    pmPhysic( const int & map_size_x, const int & map_size_y, const int & map_size_types, const int & tile_size, int * map_data ){
      msize_x = map_size_x;
      msize_y = map_size_y;
      msize_types = map_size_types;
      msize_types_x_msize_y = msize_y * msize_types;
      tsize = tile_size;
      grid_size = 2*tile_size;
      mdata = map_data;
    }

    int GetMapIndexSafe( const float & px, const float & py ){
      int ipx = int(px) / tsize;
      int ipy = int(py) / tsize;
      if(ipx < 0 or ipx > msize_x or ipy < 0 or ipy > msize_y){
        printf("pmPhysic.GetTypeSafe(): invalid request %f %f", px, py);
        return 0;
      }
      return msize_types_x_msize_y*ipx + msize_types*ipy;
    }

    int GetTypeSafe( const float & px, const float & py, const int & type_index){
      int ipx = int(px) / tsize;
      int ipy = int(py) / tsize;
      if(ipx < 0 or ipx > msize_x or ipy < 0 or ipy > msize_y or type_index < 0 or type_index > msize_types){
        printf("pmPhysic.GetTypeSafe(): invalid request %f %f %d", px, py, type_index);
        return 0;
      }
      return mdata[ msize_types_x_msize_y*ipx + msize_types*ipy + type_index ];
    }

    void Tick(const int & N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_r_sizes,
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

      // - check if objects is going to interact with solid tiles
      std::map< std::pair<int,int>, std::vector<int> > obj_grid;
      float pos_x, pos_y, speed_x, speed_y, npos_x, npos_y;
      float size_r2, nsize_r2;
      for(int i = 0, i_max = 2*N_objects; i < i_max; i+=2){
        pos_x = object_positions[ i   ];
        pos_y = object_positions[ i+1 ];

        speed_x = object_speeds[ i   ];
        speed_y = object_speeds[ i+1 ];

        npos_x = pos_x + speed_x;
        npos_y = pos_y + speed_y;

        if( GetTypeSafe(npos_x, pos_y, 0) ){
          npos_x = pos_x;
          speed_x = 0;
        }
        if( GetTypeSafe(pos_x, npos_y, 0) ){
          npos_y = pos_y;
          speed_y = 0;
        }
        if( GetTypeSafe(npos_x, npos_y, 0) ){
          speed_x = 0;
          speed_y = 0;
          npos_x = pos_x;
          npos_y = pos_y;
        }

        // => return the maximum available speed TODO
        object_positions[ i   ] = npos_x;
        object_positions[ i+1 ] = npos_y;
        object_speeds[ i   ] = speed_x;
        object_speeds[ i+1 ] = speed_y;

        // create a grid also
        size_r2 = object_r_sizes[i];
        for(int size_x = 0; size_x < size_r2; size_x += grid_size)
          for(int size_y = 0; size_y < size_r2; size_y += grid_size){
            obj_grid[std::make_pair<int, int>(int(size_x + npos_x)/grid_size, int(size_y + npos_y)/grid_size)].push_back( i );
            if(npos_x) obj_grid[std::make_pair<int, int>(int(size_x - npos_x)/grid_size, int(size_y + npos_y)/grid_size)].push_back( i );
            if(npos_y) obj_grid[std::make_pair<int, int>(int(size_x + npos_x)/grid_size, int(size_y - npos_y)/grid_size)].push_back( i );
            if(npos_x and npos_y) obj_grid[std::make_pair<int, int>(int(size_x - npos_x)/grid_size, int(size_y - npos_y)/grid_size)].push_back( i );
          }

        //   => return list of types of the tiles we interact with
        index = GetMapIndexSafe(npos_x, npos_y);
        for(type = 0; type < msize_types; type++)
          object_tiles[msize_types*i + type] = mdata[index + type];
      }

      // - check if object is going to interact with another object
      for(int i = 0, i_max = 2*N_objects; i < i_max; i+=2){
        pos_x = object_positions[ i   ];
        pos_y = object_positions[ i+1 ];
        size_r2 = pow(object_r_sizes[i], 2);
        for(int j = i+2; j < i_max; j+=2){
          npos_x = object_positions[ j   ];
          npos_y = object_positions[ j+1 ];
          nsize_r2 = pow(object_r_sizes[i], 2);
          if( pow(pos_x - npos_x, 2) + pow(pos_y - npos_y, 2) > size_r2 + nsize_r2 ) continue;
          // TODO some objects could have rectangular size rather than circle
          // => return ids of objects TODO
          interacted_object_x_object_pairs.push_back( std::make_pair<int, int>(i, j) );
        }
      }

      // - check bullet is about interation with solid tiles
      // - check if object is going to interact with bullets
      for(int i = 0, i_max = 2*N_bullets; i < i_max; i+=2){
        pos_x = bullet_positions[ i   ];
        pos_y = bullet_positions[ i+1 ];

        //   => return list of types of the tiles we interact with
        index = GetMapIndexSafe(npos_x, npos_y);
        for(type = 0; type < msize_types; type++)
          bullet_tiles[msize_types*i + type] = mdata[index + type];

        std::pair<int,int> bkey = std::make_pair<int, int>(int(pos_x)/grid_size, int(pos_y)/grid_size);
        if( obj_grid.find(bkey) == obj_grid.end() ) continue;
        const std::vector<int> & ids = obj_grid.find(bkey)->second;
        for(unsigned int id_index = 0; id_index != ids.size(); id_index++){
          npos_x = object_positions[ id_index   ];
          npos_y = object_positions[ id_index+1 ];
          nsize_r2 = pow(object_r_sizes[id_index], 2);
          if( pow(pos_x - npos_x, 2) + pow(pos_y - npos_y, 2) > nsize_r2 ) continue; // bullet is a point
          // TODO some objects could have rectangular size rather than circle
          // => return ids of objects
          interacted_bullet_x_object_pairs.push_back( std::make_pair<int, int>(i, id_index) );
        }
      }

      // - check if objects is going to interact with lines ... (such as laser, lighting traps etc)
      // TODO
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

    PyObject* PyTick(const int & N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_r_sizes,
                     const int & N_bullets, float * bullet_positions, int * bullet_tiles){
      std::vector< std::pair<int, int> > interacted_object_x_object_pairs;
      std::vector< std::pair<int, int> > interacted_bullet_x_object_pairs;
      Tick( N_objects, object_positions, object_speeds, object_tiles, object_r_sizes, N_bullets, bullet_positions, bullet_tiles, interacted_object_x_object_pairs, interacted_bullet_x_object_pairs);

      PyObject* answer = PyList_New(2);
      PyObject* interacted_object_x_object_pairs_PyList = FillPyListFromPairVector( interacted_object_x_object_pairs );
      PyObject* interacted_bullet_x_object_pairs_PyList = FillPyListFromPairVector( interacted_bullet_x_object_pairs );

      PyList_SetItem( answer, 0, interacted_object_x_object_pairs_PyList);
      PyList_SetItem( answer, 1, interacted_bullet_x_object_pairs_PyList);
      return answer;
    }


    int msize_x, msize_y, msize_types, msize_types_x_msize_y;
    int tsize, grid_size;
    int * mdata;
  };

  extern "C" {
    pmPhysic* pmPhysic_new( const int & map_size_x, const int & map_size_y, const int & map_size_types, const int & tile_size, int * map_data ) {
      return new pmPhysic( map_size_x, map_size_y, map_size_types, tile_size, map_data );
    }

    PyObject* pmPhysic_PyTick(pmPhysic* physic, const int & N_objects, float * object_positions, float * object_speeds, int * object_tiles, float * object_r_sizes, const int & N_bullets, float * bullet_positions, int * bullet_tiles) {
      return physic->PyTick(N_objects, object_positions, object_speeds, object_tiles, object_r_sizes, N_bullets, bullet_positions, bullet_tiles);
    }
  }

*/
  int check(){ return 0; }

#endif





