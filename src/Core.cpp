
#include "Core.hpp"

Core::Core(void)
{
}

Core::~Core(void)
{
	stopThreads();
	glfwDestroyWindow(window);
	glfwTerminate();
	std::cerr << "done" << std::endl;
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	Core		*core = static_cast<Core *>(glfwGetWindowUserPointer(window));

	(void)scancode;
	(void)mods;
	(void)core;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static void
cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	Core		*core = static_cast<Core *>(glfwGetWindowUserPointer(window));

	core->camera.vangle -= ((ypos - core->windowHeight * 0.5f) * 0.05);
	if (core->camera.vangle > 89.0f)
		core->camera.vangle = 89.0f;
	else if (core->camera.vangle < -89.0f)
		core->camera.vangle = -89.0f;
	core->camera.hangle -= ((xpos - core->windowWidth * 0.5f) * 0.05f);
	core->camera.hangle = fmod(core->camera.hangle, 360);
	glfwSetCursorPos(core->window, core->windowWidth * 0.5f, core->windowHeight * 0.5f);

}

void
mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	Core		*core = static_cast<Core *>(glfwGetWindowUserPointer(window));

	(void)button;
	(void)action;
	(void)core;
	(void)mods;
	// if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		// core->updateLeftClick();
}

// *************************************************************************************************

void
Core::buildProjectionMatrix(Mat4<float> &proj, float const &fov,
							float const &near, float const &far)
{
	float const			f = 1.0f / tan(fov * (M_PI / 360.0));
	float const			ratio = (1.0f * windowWidth) / windowHeight;

	proj.setIdentity();
	proj[0] = f / ratio;
	proj[1 * 4 + 1] = f;
	proj[2 * 4 + 2] = (far + near) / (near - far);
	proj[3 * 4 + 2] = (2.0f * far * near) / (near - far);
	proj[2 * 4 + 3] = -1.0f;
	proj[3 * 4 + 3] = 0.0f;
}

void
Core::getLocations(void)
{
	// attribute variables
	positionLoc = glGetAttribLocation(program, "position");
	textureLoc = glGetAttribLocation(program, "texture");
	textureIndexLoc = glGetAttribLocation(program, "textureIndex");
	// uniform variables
	colorLoc = glGetUniformLocation(program, "color");
	projLoc = glGetUniformLocation(program, "proj_matrix");
	viewLoc = glGetUniformLocation(program, "view_matrix");
	objLoc = glGetUniformLocation(program, "obj_matrix");
	renderVoxelRidgesLoc = glGetUniformLocation(program, "renderVoxelRidges");
}

GLuint
Core::loadTexture(char const *filename)
{
	GLuint				texture;
	Bmp					bmp;

	if (!bmp.load(filename))
		return (printError("Failed to load bmp !", 0));
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp.width, bmp.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	checkGlError(__FILE__, __LINE__);
	return (texture);
}

GLuint
Core::loadTextureArrayFromAtlas(char const *filename)
{
	GLuint				texture;
	Bmp					bmp;

	if (!bmp.load(filename))
		return (printError("Failed to load bmp !", 0));
	texture = 0;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0); // maybe useless
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8,
				256, 256, 10,
				0, GL_RGB, GL_UNSIGNED_BYTE, 0); // allocate the texture array
	glPixelStorei(GL_UNPACK_ROW_LENGTH, bmp.width); // set the total atlas size in order to cut through it with glTexSubImage3D
	for (int x = 0; x < 10; ++x)
	{
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
						0, 0, x, // z (using x tex coord) is the `index` of the array
						256, 256, 1, // generate 1 subtexture of 256x256
						GL_RGB, GL_UNSIGNED_BYTE, bmp.data + (x * 256) * 3); // get the pointer on the texture data using very advanced arithmetics. (index * width * RGB)
		checkGlError(__FILE__, __LINE__);
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	// glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	return (texture);
}

void
Core::loadTextures(void)
{
	tex = new GLuint[1];
	if (!(tex[0] = loadTextureArrayFromAtlas("resources/atlas.bmp")))
		exit(0);
}

void
glErrorCallback(GLenum			source,
				GLenum			type,
				GLuint			id,
				GLenum			severity,
				GLsizei			length,
				const GLchar	*message,
				GLvoid			*userParam)
{
	(void)userParam;
	(void)length;
	std::cerr << "OpenGL Error:" << std::endl;
	std::cerr << "=============" << std::endl;
	std::cerr << " Object ID: " << id << std::endl;
	std::cerr << " Severity:  " << severity << std::endl;
	std::cerr << " Type:      " << type << std::endl;
	std::cerr << " Source:    " << source << std::endl;
	std::cerr << " Message:   " << message << std::endl;
	glFinish();
}

void
Core::createSelectionCube(void)
{
	//          y
	//		    2----3
	//		   /|   /|
	//		 6----7  |
	//		 |  0-|--1   x
	//		 |/   | /
	//		 4____5
	//		z

	selectionVerticesSize = 24;
	selectionIndicesSize = 24;
	static GLfloat const		vertices[24] =
	{
		// vertices      | texture			C	I
		0.0f, 0.0f, 0.0f, // 0
		1.0f, 0.0f, 0.0f, // 1
		0.0f, 1.0f, 0.0f, // 2
		1.0f, 1.0f, 0.0f, // 3
		0.0f, 0.0f, 1.0f, // 4
		1.0f, 0.0f, 1.0f, // 5
		0.0f, 1.0f, 1.0f, // 6
		1.0f, 1.0f, 1.0f  // 7
	};

	static GLushort const		indices[24] =
	{
		0, 1,
		0, 2,
		3, 1,
		3, 2,
		4, 5,
		4, 6,
		7, 5,
		7, 6,
		2, 6,
		3, 7,
		0, 4,
		1, 5
	};

	glGenVertexArrays(1, &selectionVao);
	glBindVertexArray(selectionVao);
	glGenBuffers(2, &selectionVbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, selectionVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * selectionVerticesSize, vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(positionLoc);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void *)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, selectionVbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * selectionIndicesSize, indices, GL_STATIC_DRAW);
}

void
Core::generateChunkMesh(Chunk *chunk) const // multithread
{
	float					x, y, z, s;
	float					bt;
	float					sd;
	float					cx, cy, cz, cs;
	int						ix, iy, iz;

	cx = chunk->getCube().getX();
	cy = chunk->getCube().getY();
	cz = chunk->getCube().getZ();
	cs = chunk->getCube().getS();
	s = BLOCK_SIZE;
	for (x = cx; x < cx + cs; x += BLOCK_SIZE)
	{
		for (y = cy; y < cy + cs; y += BLOCK_SIZE)
		{
			for (z = cz; z < cz + cs; z += BLOCK_SIZE)
			{
				ix = (x - cx) / BLOCK_SIZE;
				iy = (y - cy) / BLOCK_SIZE;
				iz = (z - cz) / BLOCK_SIZE;
				bt = chunk->getBlock(ix, iy, iz).getType();
				if (bt != AIR)
				{
					if ((iy + 1 < CHUNK_SIZE && chunk->getBlock(ix, iy + 1, iz).getType() == AIR) || iy + 1 == CHUNK_SIZE) // Up
						chunk->mesh.pushUpFace(x, y, z, s, bt - 1);
					sd = bt;
					if (bt == GRASS)
						sd = DIRT;
					if ((iy - 1 >= 0 && chunk->getBlock(ix, iy - 1, iz).getType() == AIR) || iy - 1 < 0) // Bottom
						chunk->mesh.pushBottomFace(x, y, z, s, sd - 1);
					sd = bt;
					if (bt == GRASS)
						sd = SIDE_GRASS;
					if ((iz - 1 >= 0 && chunk->getBlock(ix, iy, iz - 1).getType() == AIR) || iz - 1 < 0) // Back
						chunk->mesh.pushBackFace(x, y, z, s, sd - 1);
					if ((iz + 1 < CHUNK_SIZE && chunk->getBlock(ix, iy, iz + 1).getType() == AIR) || iz + 1 == CHUNK_SIZE) // Front
						chunk->mesh.pushFrontFace(x, y, z, s, sd - 1);
					if ((ix - 1 >= 0 && chunk->getBlock(ix - 1, iy, iz).getType() == AIR) || ix - 1 < 0) // Left
						chunk->mesh.pushLeftFace(x, y, z, s, sd - 1);
					if ((ix + 1 < CHUNK_SIZE && chunk->getBlock(ix + 1, iy, iz).getType() == AIR) || ix + 1 == CHUNK_SIZE) // Right
						chunk->mesh.pushRightFace(x, y, z, s, sd - 1);
				}
			}
		}
	}
}

void
Core::generateGreedyMesh(Chunk *chunk) const
{
	(void)chunk;
}

/*void
Core::createTree(Chunk *chunk, int const &depth, float x, float y, float z) const
{
	float			tx, ty, tz;
	float			bSize;
	float			ly;

	bSize = this->block_size[(int)depth]; 

	for (ly = y; ly <= y + 2; ly += bSize)
	{
		chunk->insert(x, ly, z, depth, BLOCK, WOOD);
	}
	for (tx = x + 1; tx > x - 1.5; tx -= bSize)
	{
		for (ty = ly + 1; ty > ly - 1.5; ty -= bSize)
		{
			for (tz = z + 1; tz > z - 1.5; tz -= bSize)
			{
				if (pow(tx - x, 2) + pow(ty - ly, 2) + pow(tz - z, 2) < 1) 
				{
					chunk->insert(tx, ty, tz, depth, BLOCK, LEAF);
				}
			}

		}
	}
}
*/
void
Core::initNoises(void) // multithread
{
	noise = new Noise(42, 256);
	// octaves range     : 1.0 - 6.0
	// frequency range   : 0.0 - 1.0
	// lacunarity range  : ?
	// amplitude range   : > 0.0
	// persistence range : 0.0 - 10
	noise->configs.emplace_back(4, 0.01, 0.5, 0.1, 4.0); // bruit 3d test					//	0
	noise->configs.emplace_back(6, 0.001, 1.0, 0.9, 8.0); // bruit 3d équilibré				//	1
	noise->configs.emplace_back(2, 0.001, 10.0, 5.0, 10.0); // bruit 3d monde des reves		//	2
	noise->configs.emplace_back(3, 0.1, 0.1, 0.1, 0.2); // Des montagnes, mais pas trop		//	3
	noise->configs.emplace_back(6, 0.1, 0.0, 0.1, 10.0); // La vallée Danna					//	4
	noise->configs.emplace_back(1, 0.2, 0.0, 0.1, 4.0); // Les montagnes.					//	5
	noise->configs.emplace_back(5, 6, 0.2, 0.2, 1);		// Tree								//	6
	srandom(time(NULL));
	std::cerr	<< "octaves:     " << this->noise->configs.at(0).octaves << std::endl
				<< "frequency:   " << this->noise->configs.at(0).frequency << std::endl
				<< "lacunarity:  " << this->noise->configs.at(0).lacunarity << std::endl
				<< "amplitude:   " << this->noise->configs.at(0).amplitude << std::endl
				<< "persistence: " << this->noise->configs.at(0).persistence << std::endl;
}

void
Core::generateBlock3d(Chunk *chunk, float const &x, float const &y, float const &z, int const &ycap) const // multithread
{
	float						n;
	float						ncoal;
	float						cx, cy, cz;
	float						wx, wy, wz;
	// float						ntree;
	// float						dbSize;
	// float						bSize;
	int							i;

	wx = chunk->getCube().getX() + x;
	wy = chunk->getCube().getY() + y;
	wz = chunk->getCube().getZ() + z;

	cx = x / BLOCK_SIZE;
	cy = y / BLOCK_SIZE;
	cz = z / BLOCK_SIZE;

	// dbSize = this->block_size[depth] * 2;
	// bSize = this->block_size[depth];
	// ntree = noise->fractal(6, wx, 0, wz);

	n = 0.0f;
	ncoal = noise->fractal(5, wx, wy, wz);
	for (i = 0; i < 3; ++i)
		n += noise->octave_noise_3d(i, wx, wy, wz) * 3;
	n /= (i + 1);
	if (wy > 0)
	{
		n /= (wy / ycap);
		if (n > 0.90)
		{
			if (n < 1.0)
			{
				// if (ntree > 0.3 && chunk->search(wx, wy + dbSize, wz) != NULL
				// &&  chunk->search(wx, wy + dbSize, wz)->getState() == EMPTY)
					// createTree(chunk, depth, wx, wy + bSize, wz);
				if ((cy + 1 < CHUNK_SIZE && chunk->getBlock(cx, cy + 1, cz).getType() == AIR) || cy + 1 == CHUNK_SIZE)
					chunk->setBlock(cx, cy, cz, GRASS);
				else
					chunk->setBlock(cx, cy, cz, DIRT);
			}
			else
			{
				if ((ncoal > 0.85 && ncoal < 0.86) || (ncoal > 0.75 && ncoal < 0.76))
					chunk->setBlock(cx, cy, cz, COAL);
				else
					chunk->setBlock(cx, cy, cz, STONE);
			}
		}
	}
}

void
Core::processChunkGeneration(Chunk *chunk) // multithread
{
	float						x, z, y;

	if (chunk->getGenerated())
		return ;
	chunk->setGenerated(false);
	if (chunk->getStopGenerating())
	{
		chunk->setRenderDone(true);
		chunk->setGenerated(true);
		chunk->setRemovable(true);
		return ;
	}
	for (z = 0.0f; z < CHUNK_SIZE; z += BLOCK_SIZE)
	{
		for (x = 0.0f; x < CHUNK_SIZE; x += BLOCK_SIZE)
		{
			for (y = CHUNK_SIZE - BLOCK_SIZE; y >= 0.0f; y -= BLOCK_SIZE)
			{
				if (chunk->getStopGenerating())
				{
					chunk->setRemovable(true);
					return ;
				}
				if (chunk)
					generateBlock3d(chunk, x, y, z, 50);
			}
		}
	}
	generateChunkMesh(chunk);
	chunk->setGenerated(true);
	chunk->setRemovable(true);
}

// THREAD POOL

void *
Core::executeThread(int const &id) // multithread
{
	Chunk			*chunk;

	while (true)
	{
		// lock task queue and try to pick a task
		pthread_mutex_lock(&task_mutex[id]);
		is_task_locked[id] = true;

		// make the thread wait when the pool is empty
		while (this->pool_state != STOPPED && this->task_queue[id].empty())
			pthread_cond_wait(&this->task_cond[id], &this->task_mutex[id]);

		// stop the thread when the pool is destroyed
		if (this->pool_state == STOPPED)
		{
			// unlock to exit
			is_task_locked[id] = false;
			pthread_mutex_unlock(&task_mutex[id]);
			pthread_exit(0);
		}

		// pick task to process
		chunk = task_queue[id].front();
		if (chunk != 0)
		{
			task_queue[id].pop_front();

			// unlock task queue
			is_task_locked[id] = false;
			pthread_mutex_unlock(&task_mutex[id]);

			// process task
			processChunkGeneration(chunk);
		}
		else
		{
			is_task_locked[id] = false;
			pthread_mutex_unlock(&task_mutex[id]);
			task_queue[id].pop_front();
		}
	}
	return (0);
}

static void *
startThread(void *args) // multithread
{
	ThreadArgs *		ta = (ThreadArgs *)args;

	ta->core->executeThread(ta->i);
	delete ta;
	return (0);
}

int
Core::stopThreads(void)
{
	int				err;
	int				i;
	void			*res;

	// lock task queue
	for (i = 0; i < this->pool_size; ++i)
	{
		pthread_mutex_lock(&this->task_mutex[i]);
		this->is_task_locked[i] = true;
	}

	this->pool_state = STOPPED;

	// unlock task queue
	for (i = 0; i < this->pool_size; ++i)
	{
		this->is_task_locked[i] = false;
		pthread_mutex_unlock(&this->task_mutex[i]);
	}

	// notify threads that they need to exit
	for (i = 0; i < this->pool_size; ++i)
		pthread_cond_broadcast(&this->task_cond[i]);

	// wait for threads to exit and join them
	err = -1;
	for (i = 0; i < this->pool_size; ++i)
	{
		err = pthread_join(this->threads[i], &res);
		(void)err;
		(void)res;
		// notify threads waiting
		pthread_cond_broadcast(&this->task_cond[i]);
	}

	// destroy mutex and cond
	for (i = 0; i < this->pool_size; ++i)
	{
		pthread_mutex_destroy(&this->task_mutex[i]);
		pthread_cond_destroy(&this->task_cond[i]);
	}

	// release memory
	delete [] this->task_cond;
	delete [] this->is_task_locked;
	delete [] this->task_mutex;
	delete [] this->threads;
	delete [] this->task_queue;
	return (1);
}

uint32_t
Core::getConcurrentThreads() const
{
	// just a hint
	return (std::thread::hardware_concurrency());
}

int
Core::startThreads(void)
{
	int				err;
	int				i;
	ThreadArgs		*ta;

	this->pool_size = this->getConcurrentThreads() + 1;
	// pool_size = 1;
	if (this->pool_size <= 0)
		this->pool_size = 1;
	std::cerr << "Concurrent threads: " << this->pool_size << std::endl;
	// Thread pool heap allocation, because of variable size
	for (i = 0; i < this->pool_size; ++i)
	{
		this->task_cond = new pthread_cond_t[this->pool_size];
		this->is_task_locked = new bool[this->pool_size];
		this->task_mutex = new pthread_mutex_t[this->pool_size];
		this->threads = new pthread_t[this->pool_size];
		this->task_queue = new std::deque<Chunk *>[this->pool_size];
	}
	// mutex and cond initialization
	for (i = 0; i < this->pool_size; ++i)
	{
		pthread_mutex_init(&this->task_mutex[i], NULL);
		pthread_cond_init(&this->task_cond[i], NULL);
	}
	this->pool_state = STARTED;
	err = -1;
	for (i = 0; i < this->pool_size; ++i)
	{
		ta = new ThreadArgs();
		ta->i = i;
		ta->core = this;
		err = pthread_create(&threads[i], NULL, startThread, ta);
		if (err != 0)
		{
			std::cerr << "Failed to create Thread: " << err << std::endl;
			return (0);
		}
		else
			std::cerr << "[" << i << "] Thread created: " << std::hex << threads[i] << std::dec << std::endl;
	}
	return (1);
}

void
Core::addTask(Chunk *chunk, int const &id)
{
	// lock task queue
	pthread_mutex_lock(&task_mutex[id]);
	is_task_locked[id] = true;

	// push task in queue
	task_queue[id].push_front(chunk);

	// clear thread task queues if they exceed TASK_QUEUE_OVERFLOW
	while (task_queue[id].size() > TASK_QUEUE_OVERFLOW)
		task_queue[id].pop_back();
	// wake up a thread to process task
	pthread_cond_signal(&task_cond[id]);

	// unlock task queue
	is_task_locked[id] = false;
	pthread_mutex_unlock(&task_mutex[id]);
}

void
Core::generation(void)
{
	int							cx, cy, cz;
	int							id;
	unsigned int				min;
	int							i;
	Chunk						*chunk;

	// get new chunks inside rendering area and add them to generation queues
	for (cz = 0; cz < GEN_SIZE_Z; ++cz)
	{
		for (cy = 0; cy < GEN_SIZE_Y; ++cy)
		{
			for (cx = 0; cx < GEN_SIZE_X; ++cx)
			{
				if (chunks[cz][cy][cx] != NULL)
				{
					chunk = chunks[cz][cy][cx];
					if (!chunk->getGenerated() && !chunk->getGenerating())
					{
						id = 0;
						min = TASK_QUEUE_OVERFLOW;
						for (i = 0; i < pool_size; ++i)
						{
							if (task_queue[i].size() < min)
							{
								id = i;
								min = task_queue[i].size();
							}
						}
						chunk->setGenerating(true);
						addTask(chunk, id);
					}
					if (chunk->getGenerated() && !chunk->getRenderDone())
					{
						// wait for the chunk generation and allocate the opengl mesh
						// opengl functions cannot be called from a thread so we do it on the main thread (current OpenGL context)
						chunk->mesh.createGL(positionLoc, textureLoc, textureIndexLoc);
						chunk->setRenderDone(true);
						chunk->setGenerating(false);
						chunk->setStopGenerating(false);
						chunk->setRemovable(true);
						chunk->mesh.clear();
						// chunk->deleteBlocks();
					}
				}
			}
		}
	}
}

bool
Core::chunkInTaskPool(Chunk const *chunk) const
{
	int										i;
	std::deque<Chunk *>::const_iterator		it, ite;

	for (i = 0; i < pool_size; ++i)
	{
		if (task_queue[i].size() > 0)
		{
			it = task_queue[i].begin();
			ite = task_queue[i].end();
			while (it != ite)
			{
				if (*it == chunk)
					return (true);
				++it;
			}
		}
	}
	return (false);
}
/*
Vec3<int>
Core::getChunksDirection(Chunk *central) const
{
	int			x, y, z;
	// get the vector between the current central chunk and the new one
	for (z = 0; z < GEN_SIZE_Z; ++z)
		for (y = 0; y < GEN_SIZE_Y; ++y)
			for (x = 0; x < GEN_SIZE_X; ++x)
				if (central == chunks[z][y][x])
					return (Vec3<int>(x - center, y - center, z - center));
	return (Vec3<int>(0, 0, 0));
}
*/
void
Core::insertChunks(void)
{
	int									cx, cy, cz;
	Chunk *								chunk;
	bool								inTaskPool;
	std::list<Chunk *>::iterator		it, ite;

	for (cz = 0; cz < GEN_SIZE_Z; ++cz)
	{
		for (cy = 0; cy < GEN_SIZE_Y; ++cy)
		{
			for (cx = 0; cx < GEN_SIZE_X; ++cx)
			{
				if (chunks[cz][cy][cx] == NULL)
				{
					// place new chunks in the camera perimeter, ignoring the central chunk
					chunk = new Chunk(cx * CHUNK_SIZE, cy * CHUNK_SIZE, cz * CHUNK_SIZE, CHUNK_SIZE);
					if (chunk != 0)
					{
						inTaskPool = chunkInTaskPool(chunk);
						it = chunksRemoval.begin();
						ite = chunksRemoval.end();
						while (it != ite)
						{
							if (*it == chunk)
							{
								it = chunksRemoval.erase(it);
								break;
							}
							++it;
						}
						if (chunk != chunks[cz][cy][cx])
						{
							if (!inTaskPool)
							{
								chunk->setGenerated(false);
								chunk->setGenerating(false);
								chunk->setRenderDone(false);
								chunk->setStopGenerating(false);
								chunk->setRemovable(false);
							}
							chunk->pos.x = cx;
							chunk->pos.y = cy;
							chunk->pos.z = cz;
							chunks[cz][cy][cx] = chunk;
						}
					}
				}
			}
		}
	}
}

bool
Core::chunkInView(Chunk *chunk) const
{
	for (int z = 0; z < GEN_SIZE_Z; ++z)
		for (int y = 0; y < GEN_SIZE_Y; ++y)
			for (int x = 0; x < GEN_SIZE_X; ++x)
				if (chunks[z][y][x] == chunk)
					return (true);
	return (false);
}
/*
Block *
Core::getClosestBlock(void) const
{
	Vec3<float>			pos;
	int					i;
	int const			precision = 10;
	int const			dist = 10 * precision; // blocks max distance
	Block				*block;

	pos = camera.pos;
	for (i = 0; i < dist; ++i)
	{
		block = static_cast<Block *>(octree->search(pos.x, pos.y, pos.z, BLOCK, false));
		if (block)
			return (block);
		pos += camera.forward * (block_size[BLOCK_DEPTH] / precision);
	}
	return (NULL);
}
*/
void
Core::initChunks(void)
{
	int				x, y, z;

	for (z = 0; z < GEN_SIZE_Z; ++z)
		for (y = 0; y < GEN_SIZE_Y; ++y)
			for (x = 0; x < GEN_SIZE_X; ++x)
				chunks[z][y][x] = NULL;
	// Create initial chunk
	insertChunks();
}

int
Core::init(void)
{
	windowWidth = 1920;
	windowHeight = 1080;
	if (!glfwInit())
		return (0);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(windowWidth, windowHeight, "Voxels", NULL, NULL);
	const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(window, mode->width / 2 - windowWidth / 2, mode->height / 2 - windowHeight / 2);
	// window = glfwCreateWindow(windowWidth, windowHeight,
									// "Voxels", glfwGetPrimaryMonitor(), NULL);
	if (!window)
	{
		glfwTerminate();
		return (0);
	}
	glfwSetWindowUserPointer(window, this);
	glfwMakeContextCurrent(window); // make the opengl context of the window current on the main thread
	glfwSwapInterval(1); // VSYNC 60 fps max
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// glfwDisable(GLFW_MOUSE_CURSOR);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	// cl.init();
	float const fov = 53.13f;
	float const aspect = windowWidth * 1.0f / windowHeight;
	float const near = 0.1f;
	float const far = 1000.0f;
	buildProjectionMatrix(projMatrix, fov, near, far);
	camera.init(CHUNK_SIZE * GEN_SIZE_X / 2, CHUNK_SIZE * GEN_SIZE_Y, CHUNK_SIZE * GEN_SIZE_Z / 2, fov, aspect, near, far);
	if (!initShaders(vertexShader, fragmentShader, program))
		return (0);
	getLocations();
/*#ifndef __APPLE__ // Mac osx doesnt support opengl 4.3+
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_FALSE);
	glDebugMessageCallback((GLDEBUGPROC)glErrorCallback, NULL);
#endif*/
	initNoises();
	loadTextures();
	createSelectionCube();
	startThreads();
	initChunks();
	closestBlock = 0;
	frameRenderedTriangles = 0;
	return (1);
}
/*
void
Core::updateLeftClick(void)
{
	Chunk			*chunk;

	if (closestBlock != NULL)
	{
		chunk = closestBlock->getChunk();
		closestBlock->remove();
		std::cerr << chunk->mesh.getPrimitives() << std::endl;
		chunk->mesh.clear();
		chunk->mesh.deleteGL();
		std::cerr << chunk->mesh.getPrimitives() << std::endl;
		generateChunkMesh(chunk, chunk);
		processChunkSimplification(chunk);
		std::cerr << chunk->mesh.getPrimitives() << std::endl;
		chunk->mesh.createGL(positionLoc, textureLoc, textureIndexLoc);
		chunk->mesh.clear();
		std::cerr << chunk->mesh.getPrimitives() << std::endl;
	}
}
*/
void
Core::update(void)
{
	// std::cerr << "remove list: " << chunksRemoval.size() << std::endl;
	glfwSetCursorPos(window, windowWidth / 2.0f, windowHeight / 2.0f);
	camera.rotate();
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		camera.enableBoost();
	else
		camera.disableBoost();
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.moveForward();
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.moveBackward();
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.strafeLeft();
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.strafeRight();
	generation();
	// closestBlock = getClosestBlock();
}

void
Core::render(void)
{
	float		ftime = glfwGetTime();
	int			x, y, z;
	size_t		t;

	(void)ftime;
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, camera.view.val);
	ms.push();
		// render chunks ridges
/*		glBindVertexArray(selectionVao);
		glUniform1f(renderVoxelRidgesLoc, 1.0f);
		glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
		for (z = 0; z < GEN_SIZE_Z; ++z)
		{
			for (y = 0; y < GEN_SIZE_Y; ++y)
			{
				for (x = 0; x < GEN_SIZE_X; ++x)
				{
					if (chunks[z][y][x] != NULL)
						chunks[z][y][x]->renderRidges(*this);
				}
			}
		}*/
		// if (closestBlock != NULL)
			// closestBlock->renderRidges(*this);
		// render meshes
		glUniform1f(renderVoxelRidgesLoc, 0.0f);
		t = 0;
		for (z = 0; z < GEN_SIZE_Z; ++z)
		{
			for (y = 0; y < GEN_SIZE_Y; ++y)
			{
				for (x = 0; x < GEN_SIZE_X; ++x)
				{
					if (chunks[z][y][x] != NULL)
					{
						if (camera.cubeInFrustrum(chunks[z][y][x]->getCube()) == INSIDE)
						{
							chunks[z][y][x]->render(*this);
							t += chunks[z][y][x]->mesh.getPrimitives();
						}
					}
				}
			}
		}
		frameRenderedTriangles = t;
	ms.pop();
	glfwSwapBuffers(window);
}

void
Core::loop(void)
{
	double		lastTime, currentTime;
	double		frames;
	double		tmpTime;
	double		updateTime;
	double		renderTime;

	frames = 0.0;
	lastTime = glfwGetTime();
	glUseProgram(program);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.val);
	while (!glfwWindowShouldClose(window))
	{
		currentTime = glfwGetTime();
		frames += 1.0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		tmpTime = glfwGetTime();
		update();
		updateTime = glfwGetTime() - tmpTime;

		tmpTime = glfwGetTime();
		render();
		renderTime = glfwGetTime() - tmpTime;

		glfwPollEvents();
		if (currentTime - lastTime >= 1.0)
		{
			std::string timers = std::to_string(updateTime) + " / " + std::to_string(renderTime) + " / " + std::to_string(frameRenderedTriangles) + " triangles";
			glfwSetWindowTitle(window, (std::to_string((int)frames) + " fps [" + timers + "]").c_str());
			frames = 0.0;
			lastTime += 1.0;
		}
	}
}
