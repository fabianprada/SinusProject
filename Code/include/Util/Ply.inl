/* -*- C++ -*-
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#include <hash_map>
static char *full_elem_names[] = { "vertex", "edge" , "face" };
static char *elem_names[] = { "vertex", "face" };

struct PlyEdge{ int v1 , v2; };
typedef struct PlyFace
{
	unsigned int nr_vertices;
	int *vertices;
} PlyFace;
typedef struct PlyColorFace
{
	unsigned int nr_vertices;
	int *vertices;
	float r , g , b;
} PlyColorFace;
typedef struct PlyStrip
{
	unsigned int nr_vertices;
	int *vertices;
} PlyStrip;

static PlyProperty edge_props[] = { 
	{ "v1" , PLY_INT , PLY_INT , (int)offsetof( PlyEdge , v1 ) , 0 , 0 , 0 , 0 },
	{ "v2" , PLY_INT , PLY_INT , (int)offsetof( PlyEdge , v2 ) , 0 , 0 , 0 , 0 }
};
static PlyProperty face_props[] =
{
	//{ "vertex_indices" , PLY_INT , PLY_INT , offsetof( PlyFace , vertices ) , 1 , PLY_INT , PLY_INT , offsetof( PlyFace , nr_vertices ) } ,
	//Modified PLY_INT to PLY_UCHAR. 1/3/2016.
	{ "vertex_indices", PLY_INT, PLY_INT, offsetof(PlyFace, vertices), 1, PLY_UCHAR, PLY_INT, offsetof(PlyFace, nr_vertices) },
};
static PlyProperty color_face_props[] =
{
	{ "vertex_indices" , PLY_INT , PLY_INT , offsetof( PlyFace , vertices ) , 1 , PLY_INT , PLY_INT , offsetof( PlyFace , nr_vertices ) } ,
	{ "red"   , PLY_FLOAT , PLY_FLOAT , offsetof( PlyColorFace , r ) , 0 , 0 , 0 , 0 } ,
	{ "green" , PLY_FLOAT , PLY_FLOAT , offsetof( PlyColorFace , g ) , 0 , 0 , 0 , 0 } ,
	{ "blue"  , PLY_FLOAT , PLY_FLOAT , offsetof( PlyColorFace , b ) , 0 , 0 , 0 , 0 } ,
};
static PlyProperty strip_props[] =
{
	{ "vertex_indices" , PLY_INT , PLY_INT , offsetof( PlyFace , vertices ) , 1 , PLY_INT , PLY_INT , offsetof( PlyFace , nr_vertices )	},
};

template< class Vertex >
int PlyWrite
	(
	const char* fileName,
	const std::vector< Vertex >& vertices , 
	const std::vector< std::pair< int , int > >* edges , 
	const std::vector< std::vector< int > >* polygons,
	PlyProperty* properties , int propertyNum ,
	int file_type ,
	char** comments , const int& commentNum
	)
{
	int nr_vertices =            (int) vertices.size()    ;
	int nr_edges    = edges    ? (int)   edges->size() : 0;
	int nr_faces    = polygons ? (int)polygons->size() : 0;
	float version;
	PlyFile *ply = ply_open_for_writing( fileName , 3 , full_elem_names , file_type , &version );
	if( !ply ) return 0;
	
	//
	// describe vertex, edge, and face properties
	//
	{
		ply_element_count( ply , "vertex" , nr_vertices );
		for( int i=0 ; i<propertyNum ; i++ ) ply_describe_property( ply , "vertex" ,  &properties[i] );
	}
	{
		ply_element_count( ply , "edge" , nr_edges );
		ply_describe_property( ply , "edge" , &edge_props[0] );
		ply_describe_property( ply , "edge" , &edge_props[1] );
	}
	{
		ply_element_count( ply , "face" , nr_faces );
		ply_describe_property( ply , "face" , &face_props[0] );
	}
	
	// Write in the comments
	if( comments && commentNum ) for( int i=0 ; i<commentNum ; i++ ) ply_put_comment( ply , comments[i] );

	ply_header_complete( ply );
	
	// write vertices
	ply_put_element_setup( ply , "vertex" );
	for( int i=0 ; i<nr_vertices ; i++ ) ply_put_element( ply , (void*)&vertices[i] );

	// write edges
	if( nr_edges )
	{
		PlyEdge ply_edge;
		ply_put_element_setup( ply , "edge" );
		for( int i=0 ; i<nr_edges ; i++ )
		{
			ply_edge.v1 = (*edges)[i].first , ply_edge.v2 = (*edges)[i].second;
			ply_put_element( ply , (void*)&ply_edge );
		}
	}

	// write faces
	if( nr_faces )
	{
		PlyFace ply_face;
		int maxFaceVerts=3;
		ply_face.nr_vertices = 3;
		ply_face.vertices = new int[3];

		ply_put_element_setup( ply , "face" );
		for( int i=0 ; i<nr_faces ; i++ )
		{
			int face_size = (int)(*polygons)[i].size();
			if( face_size>maxFaceVerts )
			{
				delete[] ply_face.vertices;
				maxFaceVerts = face_size;
				ply_face.vertices = new int[face_size];
			}
			ply_face.nr_vertices = face_size;
			for( unsigned int j=0 ; j<ply_face.nr_vertices ; j++ ) ply_face.vertices[j] = (*polygons)[i][j];
			ply_put_element( ply , (void*)&ply_face );
		}
		delete[] ply_face.vertices;
	}
	ply_close( ply );
	return 1;
}
template< class Vertex >
int PlyRead
	(
	const char* fileName ,
	std::vector< Vertex >& vertices , 
	std::vector< std::pair< int , int > >* edges ,
	std::vector< std::vector< int > >* polygons ,
	PlyProperty* properties , bool* propertiesFlag , int propertyNum ,
	int& file_type ,
	char*** comments , int* commentNum )
{
	int nr_elems;
	char **elist;
	float version;
	int i,j,k;
	PlyFile* ply;
	char* elem_name;
	int num_elems;
	int nr_props;
	PlyProperty** plist;
	PlyFace ply_face;
	int width , height;

	ply = ply_open_for_reading( fileName , &nr_elems , &elist , &file_type , &version );
	if( !ply ) return 0;

	if( comments )
	{
		(*comments) = new char*[*commentNum+ply->num_comments];
		for(int i=0;i<ply->num_comments;i++) (*comments)[i]=_strdup(ply->comments[i]);
		*commentNum=ply->num_comments;
	}

	for( i=0 ; i<nr_elems; i++ )
	{
		elem_name = elist[i];
		plist = ply_get_element_description( ply , elem_name , &num_elems , &nr_props );

		if( !plist )
		{
			for( i=0 ; i<nr_elems ; i++ )
			{
				free( ply->elems[i]->name );
				free( ply->elems[i]->store_prop );
				for( j=0 ; j<ply->elems[i]->nprops ; j++ )
				{
					free( ply->elems[i]->props[j]->name );
					free( ply->elems[i]->props[j] );
				}
				free( ply->elems[i]->props );
			}
			for( i=0 ; i<nr_elems ; i++ ) free(ply->elems[i]);
			free( ply->elems );
			for( i=0 ; i<ply->num_comments ; i++ ) free( ply->comments[i] );
			free( ply->comments );
			for( i=0 ; i<ply->num_obj_info ; i++ ) free(ply->obj_info[i]);
			free( ply->obj_info );
			ply_free_other_elements( ply->other_elems );
			
			for( i=0 ; i<nr_elems ; i++ ) free(elist[i]);
			free( elist );
			ply_close( ply );
			return 0;
		}
		if( equal_strings( "vertex" , elem_name ) )
		{
			for( int i=0 ; i<propertyNum ; i++ )
			{
				bool hasProperty = ply_get_property( ply , elem_name , &properties[i] );
				if( propertiesFlag ) propertiesFlag[i] = hasProperty;
			}
			vertices.resize( num_elems );
			for( j=0 ; j<num_elems ; j++ ) ply_get_element( ply , (void*)&vertices[j] );
		}
		else if( equal_strings( "edge" , elem_name ) && edges )
		{
			ply_get_property( ply , elem_name , &edge_props[0] );
			ply_get_property( ply , elem_name , &edge_props[1] );
			edges->resize( num_elems );
			for( j=0 ; j<num_elems ; j++ )
			{
				PlyEdge ply_edge;
				ply_get_element( ply , (void*)&ply_edge );
				(*edges)[j].first = ply_edge.v1 , (*edges)[j].second = ply_edge.v2;
			}
		}
		else if( equal_strings( "face" , elem_name ) && polygons )
		{
			ply_get_property( ply , elem_name , &face_props[0] );
			polygons->resize( num_elems );
			for( j=0 ; j<num_elems ; j++ )
			{
				ply_get_element( ply , (void*)&ply_face );
				(*polygons)[j].resize( ply_face.nr_vertices );
				for( k=0 ; k<(int)ply_face.nr_vertices ; k++ ) (*polygons)[j][k] = ply_face.vertices[k];
				delete[] ply_face.vertices;
			}  // for, read faces
		}  // if face
		else if( equal_strings( "tristrips" , elem_name ) && polygons )
		{
			PlyStrip ply_strip;
			for( int j=0 ; j<num_elems ; j++ )
			{
				ply_get_property( ply , elem_name , &strip_props[0] );
				ply_get_element( ply , (void *) &ply_strip );
				int idx = 0;
				for( unsigned int k=0 ; k<ply_strip.nr_vertices - 2 ; k++ )
				{
					int i0 = ply_strip.vertices[k+0];
					int i1 = ply_strip.vertices[k+1];
					int i2 = ply_strip.vertices[k+2];
					if( i0>=0 && i1>=0 && i2>=0 )
					{
						size_t sz = polygons->size();
						polygons->resize( sz + 1 );
						(*polygons)[sz].resize( 3 );
						if( !(idx&1 ) )
						{
							(*polygons)[sz][0] = i0;
							(*polygons)[sz][1] = i1;
							(*polygons)[sz][2] = i2;
						}
						else
						{
							(*polygons)[sz][1] = i0;
							(*polygons)[sz][0] = i1;
							(*polygons)[sz][2] = i2;
						}
						idx++;
//						(*polygons)[sz][0] = i0;
//						(*polygons)[sz][1] = i1;
//						(*polygons)[sz][2] = i2;
					}
				}
				delete[] ply_strip.vertices;
			}  // for, read triangle strips
		}
		else ply_get_other_element( ply , elem_name , num_elems );

		for( j=0 ; j<nr_props ; j++ ) free( plist[j]->name ) , free( plist[j] );

		free( plist );
	}  // for each type of element
	
	for( i=0 ; i<nr_elems ; i++ )
	{
		free( ply->elems[i]->name );
		free( ply->elems[i]->store_prop );
		for( j=0 ; j<ply->elems[i]->nprops ; j++ ) free( ply->elems[i]->props[j]->name ) , free( ply->elems[i]->props[j] );
		if( ply->elems[i]->props && ply->elems[i]->nprops ) free(ply->elems[i]->props);
	}
	for( i=0 ; i<nr_elems ; i++ ) free( ply->elems[i] );
	free( ply->elems );
	for( i=0 ; i<ply->num_comments ; i++ ) free( ply->comments[i] );
	free( ply->comments );
	for( i=0 ; i<ply->num_obj_info ; i++ ) free( ply->obj_info[i] );
	free( ply->obj_info );
	ply_free_other_elements( ply->other_elems );
	
	
	for( i=0 ; i<nr_elems ; i++ ) free( elist[i] );
	free( elist );
	ply_close( ply );
	return 1;
}
template<class Vertex>
int PlyWritePolygons( const char* fileName,
					 const std::vector<Vertex>& vertices,const std::vector< std::vector<int> >& polygons,
					 PlyProperty* properties,int propertyNum,
					 int file_type,
					 char** comments,const int& commentNum)
{
	int nr_vertices=int(vertices.size());
	int nr_faces=int(polygons.size());
	float version;
	PlyFile *ply = ply_open_for_writing(fileName, 2, elem_names, file_type, &version);
	if (!ply){return 0;}
	
	//
	// describe vertex and face properties
	//
	ply_element_count(ply, "vertex", nr_vertices);
	for(int i=0;i<propertyNum;i++)
		ply_describe_property(ply, "vertex", &properties[i]);
	
	ply_element_count(ply, "face", nr_faces);
	ply_describe_property(ply, "face", &face_props[0]);
	
	// Write in the comments
	if(comments && commentNum)
		for(int i=0;i<commentNum;i++)
			ply_put_comment(ply,comments[i]);

	ply_header_complete(ply);
	
	// write vertices
	ply_put_element_setup(ply, "vertex");
	for (int i=0; i < int(vertices.size()); i++)
		ply_put_element(ply, (void *) &vertices[i]);

	// write faces
	PlyFace ply_face;
	int maxFaceVerts=3;
	ply_face.nr_vertices = 3;
	ply_face.vertices = new int[3];

	ply_put_element_setup(ply, "face");
	for (int i=0; i < nr_faces; i++)
	{
		if(int(polygons[i].size())>maxFaceVerts)
		{
			delete[] ply_face.vertices;
			maxFaceVerts=int(polygons[i].size());
			ply_face.vertices=new int[maxFaceVerts];
		}
		ply_face.nr_vertices=int(polygons[i].size());
		for( unsigned int j=0 ; j<ply_face.nr_vertices ; j++ ) ply_face.vertices[j]=polygons[i][j];
		ply_put_element(ply, (void *) &ply_face);
	}

	delete[] ply_face.vertices;
	ply_close(ply);
	return 1;
}



template<class Vertex>
int PlyWritePolygonsAndColor( const char* fileName,
					 const std::vector<Vertex>& vertices,const std::vector< std::vector<int> >& polygons,
					 PlyProperty* properties,int propertyNum,
					 int file_type,
					 char** comments,const int& commentNum)
{
	int nr_vertices=int(vertices.size());
	int nr_faces=int(polygons.size());
	float version;
	PlyFile *ply = ply_open_for_writing(fileName, 2, elem_names, file_type, &version);
	if (!ply){return 0;}
	
	//
	// describe vertex and face properties
	//
	ply_element_count(ply, "vertex", nr_vertices);
	for(int i=0;i<propertyNum;i++)
		ply_describe_property(ply, "vertex", &properties[i]);
	
	ply_element_count(ply, "face", nr_faces);
	ply_describe_property(ply, "face", &face_props[0]);
	
	// Write in the comments
	if(comments && commentNum)
		for(int i=0;i<commentNum;i++)
			ply_put_comment(ply,comments[i]);

	ply_header_complete(ply);
	
	// write vertices
	ply_put_element_setup(ply, "vertex");
	for (int i=0; i < int(vertices.size()); i++)
		ply_put_element(ply, (void *) &vertices[i]);

	// write faces
	PlyFace ply_face;
	int maxFaceVerts=3;
	ply_face.nr_vertices = 3;
	ply_face.vertices = new int[3];

	ply_put_element_setup(ply, "face");
	for (int i=0; i < nr_faces; i++)
	{
		if(int(polygons[i].size())>maxFaceVerts)
		{
			delete[] ply_face.vertices;
			maxFaceVerts=int(polygons[i].size());
			ply_face.vertices=new int[maxFaceVerts];
		}
		ply_face.nr_vertices=int(polygons[i].size());
		for(int j=0;j<ply_face.nr_vertices;j++)
			ply_face.vertices[j]=polygons[i][j];
		ply_put_element(ply, (void *) &ply_face);
	}

	delete[] ply_face.vertices;
	ply_close(ply);
	return 1;
}


template<class Vertex>
int PlyReadPolygons( const char* fileName,
					std::vector<Vertex>& vertices,std::vector<std::vector<int> >& polygons,
					 PlyProperty* properties , bool* propertiesFlag , int propertyNum,
					int& file_type,
					char*** comments,int* commentNum)
{
	int nr_elems;
	char **elist;
	float version;
	int i,j,k;
	PlyFile* ply;
	char* elem_name;
	int num_elems;
	int nr_props;
	PlyProperty** plist;
	PlyFace ply_face;

	ply = ply_open_for_reading( fileName , &nr_elems , &elist , &file_type , &version);
	if(!ply) return 0;

	if( comments )
	{
		(*comments) = new char*[*commentNum+ply->num_comments];
		for(int i=0;i<ply->num_comments;i++) (*comments)[i]=_strdup(ply->comments[i]);
		*commentNum=ply->num_comments;
	}

	for (i=0; i < nr_elems; i++)
	{
		elem_name = elist[i];
		plist = ply_get_element_description(ply, elem_name, &num_elems, &nr_props);
		if(!plist)
		{
			for(i=0;i<nr_elems;i++){
				free(ply->elems[i]->name);
				free(ply->elems[i]->store_prop);
				for(j=0;j<ply->elems[i]->nprops;j++){
					free(ply->elems[i]->props[j]->name);
					free(ply->elems[i]->props[j]);
				}
				free(ply->elems[i]->props);
			}
			for(i=0;i<nr_elems;i++){free(ply->elems[i]);}
			free(ply->elems);
			for(i=0;i<ply->num_comments;i++){free(ply->comments[i]);}
			free(ply->comments);
			for(i=0;i<ply->num_obj_info;i++){free(ply->obj_info[i]);}
			free(ply->obj_info);
			ply_free_other_elements (ply->other_elems);
			
			for(i=0;i<nr_elems;i++){free(elist[i]);}
			free(elist);
			ply_close(ply);
			return 0;
		}		
		if (equal_strings("vertex", elem_name))
		{
			for(int i=0;i<propertyNum;i++)
			{
				bool hasProperty = ply_get_property( ply , elem_name , &properties[i] );
				if( propertiesFlag ) propertiesFlag[i] = hasProperty;
			}
			vertices.resize(num_elems);
			for (j=0; j < num_elems; j++)	ply_get_element (ply, (void *) &vertices[j]);
		}
		else if (equal_strings("face", elem_name))
		{
			ply_get_property( ply , elem_name, &face_props[0] );
			polygons.resize( num_elems );
			for ( j=0 ; j<num_elems ; j++ )
			{
				ply_get_element( ply, (void *) &ply_face );
				polygons[j].resize( ply_face.nr_vertices );
				for( k=0 ; k<(int)ply_face.nr_vertices ; k++ ) polygons[j][k] = ply_face.vertices[k];
				delete[] ply_face.vertices;
			}  // for, read faces
		}  // if face
		else if ( equal_strings( "tristrips" , elem_name ) )
		{
			PlyStrip ply_strip;
			for( int j=0 ; j<num_elems ; j++ )
			{
				ply_get_property( ply , elem_name , &strip_props[0] );
				ply_get_element( ply , (void *) &ply_strip );
				int idx = 0;
				for( unsigned int k=0 ; k<ply_strip.nr_vertices - 2 ; k++ )
				{
					int i0 = ply_strip.vertices[k+0];
					int i1 = ply_strip.vertices[k+1];
					int i2 = ply_strip.vertices[k+2];
					if( i0>=0 && i1>=0 && i2>=0 )
					{
						size_t sz = polygons.size();
						polygons.resize( sz + 1 );
						polygons[sz].resize( 3 );
						if( !(idx&1 ) )
						{
							polygons[sz][0] = i0;
							polygons[sz][1] = i1;
							polygons[sz][2] = i2;
						}
						else
						{
							polygons[sz][1] = i0;
							polygons[sz][0] = i1;
							polygons[sz][2] = i2;
						}
						idx++;
//						polygons[sz][0] = i0;
//						polygons[sz][1] = i1;
//						polygons[sz][2] = i2;
					}
				}
				delete[] ply_strip.vertices;
			}  // for, read triangle strips
		}
		else{ply_get_other_element (ply, elem_name, num_elems);}

		for(j=0;j<nr_props;j++){
			free(plist[j]->name);
			free(plist[j]);
		}
		free(plist);
	}  // for each type of element
	
	for(i=0;i<nr_elems;i++){
		free(ply->elems[i]->name);
		free(ply->elems[i]->store_prop);
		for(j=0;j<ply->elems[i]->nprops;j++){
			free(ply->elems[i]->props[j]->name);
			free(ply->elems[i]->props[j]);
		}
		if(ply->elems[i]->props && ply->elems[i]->nprops){free(ply->elems[i]->props);}
	}
	for(i=0;i<nr_elems;i++){free(ply->elems[i]);}
	free(ply->elems);
	for(i=0;i<ply->num_comments;i++){free(ply->comments[i]);}
	free(ply->comments);
	for(i=0;i<ply->num_obj_info;i++){free(ply->obj_info[i]);}
	free(ply->obj_info);
	ply_free_other_elements (ply->other_elems);
	
	
	for(i=0;i<nr_elems;i++){free(elist[i]);}
	free(elist);
	ply_close(ply);
	return 1;
}


template<class Vertex>
int PlyReadPoints( const char* fileName,
				  std::vector<Vertex>& vertices,
				  PlyProperty* properties, bool* propertiesFlag , int propertyNum,
				  int& file_type,
				  char*** comments,int* commentNum)
{
	int nr_elems;
	char **elist;
	float version;
	int i,j;
	PlyFile* ply;
	char* elem_name;
	int num_elems;
	int nr_props;
	PlyProperty** plist;
	PlyFace ply_face;

	ply = ply_open_for_reading(fileName, &nr_elems, &elist, &file_type, &version);
	if(!ply) return 0;

	if(comments)
	{
		(*comments)=new char*[*commentNum+ply->num_comments];
		for(int i=0;i<ply->num_comments;i++) (*comments)[i]=_strdup(ply->comments[i]);
		*commentNum=ply->num_comments;
	}

	for (i=0; i < nr_elems; i++) {
		elem_name = elist[i];
		plist = ply_get_element_description(ply, elem_name, &num_elems, &nr_props);
		if(!plist)
		{
			for(i=0;i<nr_elems;i++){
				free(ply->elems[i]->name);
				free(ply->elems[i]->store_prop);
				for(j=0;j<ply->elems[i]->nprops;j++){
					free(ply->elems[i]->props[j]->name);
					free(ply->elems[i]->props[j]);
				}
				free(ply->elems[i]->props);
			}
			for(i=0;i<nr_elems;i++){free(ply->elems[i]);}
			free(ply->elems);
			for(i=0;i<ply->num_comments;i++){free(ply->comments[i]);}
			free(ply->comments);
			for(i=0;i<ply->num_obj_info;i++){free(ply->obj_info[i]);}
			free(ply->obj_info);
			ply_free_other_elements (ply->other_elems);
			
			for(i=0;i<nr_elems;i++){free(elist[i]);}
			free(elist);
			ply_close(ply);
			return 0;
		}		
		if (equal_strings("vertex", elem_name))
		{
			for(int i=0;i<propertyNum;i++)
			{
				bool hasProperty = ply_get_property( ply , elem_name,&properties[i] );
				if( propertiesFlag ) propertiesFlag[i] = hasProperty;
			}
			vertices.resize(num_elems);
			for (j=0; j < num_elems; j++)	ply_get_element (ply, (void *) &vertices[j]);
		}
		else{ply_get_other_element (ply, elem_name, num_elems);}

		for(j=0;j<nr_props;j++){
			free(plist[j]->name);
			free(plist[j]);
		}
		free(plist);
	}  // for each type of element
	
	for(i=0;i<nr_elems;i++){
		free(ply->elems[i]->name);
		free(ply->elems[i]->store_prop);
		for(j=0;j<ply->elems[i]->nprops;j++){
			free(ply->elems[i]->props[j]->name);
			free(ply->elems[i]->props[j]);
		}
		if(ply->elems[i]->props && ply->elems[i]->nprops){free(ply->elems[i]->props);}
	}
	for(i=0;i<nr_elems;i++){free(ply->elems[i]);}
	free(ply->elems);
	for(i=0;i<ply->num_comments;i++){free(ply->comments[i]);}
	free(ply->comments);
	for(i=0;i<ply->num_obj_info;i++){free(ply->obj_info[i]);}
	free(ply->obj_info);
	ply_free_other_elements (ply->other_elems);
	
	
	for(i=0;i<nr_elems;i++){free(elist[i]);}
	free(elist);
	ply_close(ply);
	return 1;
}
template<class Vertex>
int PlyWritePoints( const char* fileName,
				   const std::vector<Vertex>& vertices,
				   PlyProperty* properties,int propertyNum,
				   int file_type,
				   char** comments,const int& commentNum)
{
	int nr_vertices=int(vertices.size());
	float version;
	PlyFile *ply = ply_open_for_writing(fileName, 2, elem_names, file_type, &version);
	if (!ply){return 0;}
	
	//
	// describe vertex and face properties
	//
	ply_element_count(ply, "vertex", nr_vertices);
	for(int i=0;i<propertyNum;i++)	ply_describe_property(ply, "vertex", &properties[i]);
		
	// Write in the comments
	if(comments && commentNum)
		for(int i=0;i<commentNum;i++)
			ply_put_comment(ply,comments[i]);

	ply_header_complete(ply);
	
	// write vertices
	ply_put_element_setup(ply, "vertex");
	for (int i=0; i < int(vertices.size()); i++)	ply_put_element(ply, (void *) &vertices[i]);

	ply_close(ply);
	return 1;
}


template< class Vertex >
int PlyReadTriangles( const char* fileName ,
					  std::vector<Vertex>& vertices , std::vector< TriangleIndex >& triangles ,
					  PlyProperty* properties , bool* propertiesFlag , int propertyNum ,
					  int& file_type ,
					  char*** comments , int* commentNum )
{
	MinimalAreaTriangulation< double > MAT;
	std::vector< std::vector< int > > polygons;
	int ret = PlyReadPolygons( fileName , vertices , polygons , properties , propertiesFlag , propertyNum , file_type , comments , commentNum );
	std::vector< Point3D< double > > poly;
	std::vector< TriangleIndex > tris;

	triangles.clear();
	for (unsigned int i = 0; i < polygons.size(); i++) {
		poly.resize( polygons[i].size( ) );
		for(unsigned int j=0 ; j<polygons[i].size(); j++) poly[j] = Point3D<double>( vertices[polygons[i][j]] );
		MAT.GetTriangulation( poly , tris );
		for (unsigned int j = 0; j < tris.size(); j++)   {
			TriangleIndex tri;
			tri[0] = polygons[i][ tris[j][0] ];
			tri[1] = polygons[i][ tris[j][1] ];
			tri[2] = polygons[i][ tris[j][2] ];
			triangles.push_back( tri );
		}
	}
	return ret;
}
template<class Vertex>
int PlyWriteTriangles( const char* fileName ,
					   const std::vector< Vertex >& vertices , const std::vector< TriangleIndex >& triangles ,
					   PlyProperty* properties , int propertyNum ,
					   int file_type ,
					   char** comments , const int& commentNum)
{
	int nr_vertices=int( vertices.size() );
	int nr_faces=int( triangles.size() );
	float version;
	PlyFile *ply = ply_open_for_writing( fileName , 2 , elem_names , file_type , &version );
	if ( !ply ) return 0;
	
	ply_element_count( ply , "vertex" , nr_vertices );
	for( int i=0 ; i<propertyNum ; i++ ) ply_describe_property( ply , "vertex" , &properties[i] );
	
	ply_element_count( ply , "face" , nr_faces );
	ply_describe_property( ply , "face" , &face_props[0] );
	
	// Write in the comments
	if( comments && commentNum ) for( int i=0 ; i<commentNum ; i++ ) ply_put_comment( ply , comments[i] );

	ply_header_complete(ply);
	
	// write vertices
	ply_put_element_setup( ply , "vertex" );
	for(unsigned int i=0 ; i<vertices.size() ; i++ ) ply_put_element( ply , (void *) &vertices[i] );

	// write faces
	PlyFace ply_face;
	ply_face.nr_vertices = 3;
	ply_face.vertices = new int[3];

	ply_put_element_setup( ply , "face" );
	for (int i=0; i < nr_faces; i++)
	{
		ply_face.nr_vertices = 3;
		for( int j=0 ;j<3; j++ ) ply_face.vertices[j] = triangles[i][j];
		ply_put_element( ply, (void *) &ply_face );
	}

	delete[] ply_face.vertices;
	ply_close( ply );
	return 1;
}
template< class Vertex >
int PlyWriteColorTriangles
(
const char* fileName ,
const std::vector< Vertex >& vertices , const std::vector< std::pair< TriangleIndex , Point3D< float > > >& triangles ,
PlyProperty* properties , int propertyNum ,
int file_type ,
char** comments , const int& commentNum
)
{
	int nr_vertices=int( vertices.size() );
	int nr_faces=int( triangles.size() );
	float version;
	PlyFile *ply = ply_open_for_writing( fileName , 2 , elem_names , file_type , &version );
	if ( !ply ) return 0;
	
	ply_element_count( ply , "vertex" , nr_vertices );
	for( int i=0 ; i<propertyNum ; i++ ) ply_describe_property( ply , "vertex" , &properties[i] );
	
	ply_element_count( ply , "face" , nr_faces );
	for( int i=0 ; i<4 ; i++ ) ply_describe_property( ply , "face" , &color_face_props[i] );
	
	// Write in the comments
	if( comments && commentNum ) for( int i=0 ; i<commentNum ; i++ ) ply_put_comment( ply , comments[i] );

	ply_header_complete( ply );
	
	// write vertices
	ply_put_element_setup( ply , "vertex" );
	for(unsigned int i=0 ; i<vertices.size() ; i++ ) ply_put_element( ply , (void *) &vertices[i] );

	// write faces
	PlyColorFace ply_face;
	ply_face.nr_vertices = 3;
	ply_face.vertices = new int[3];

	ply_put_element_setup( ply , "face" );
	for ( int i=0 ; i < nr_faces ; i++ )
	{
		ply_face.nr_vertices = 3;
		for( int j=0 ; j< 3; j++ ) ply_face.vertices[j] = triangles[i].first[j];
		ply_face.r = triangles[i].second[0];
		ply_face.g = triangles[i].second[1];
		ply_face.b = triangles[i].second[2];
		ply_put_element( ply, (void *) &ply_face );
	}

	delete[] ply_face.vertices;
	ply_close( ply );
	return 1;
}