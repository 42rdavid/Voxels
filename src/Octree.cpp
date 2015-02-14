
#include "Constants.hpp"
#include "Octree.hpp"

uint32_t
Octree::ground_depth = GROUND_DEPTH;

uint32_t
Octree::chunk_depth = CHUNK_DEPTH;

Octree::Octree(void)
	: _generated(false), _state(0), _cube(), _parent(NULL)
{
	for (uint32_t i = 0; i < CHD_MAX; ++i)
		this->_children[i] = NULL;
	return ;
}

Octree::Octree(Octree const &src)
	: _generated(false), _state(src.getState()), _cube(src.getCube()), _parent(NULL)
{
	for (uint32_t i = 0; i < CHD_MAX; ++i)
		this->_children[i] = NULL;
	return ;
}

Octree::Octree(float const &x, float const &y, float const &z, float const &s)
	: _generated(false), _state(0), _cube(x, y, z, s), _parent(NULL)
{
	for (uint32_t i = 0; i < CHD_MAX; ++i)
		this->_children[i] = NULL;
	return ;
}

Octree::~Octree(void)
{
	for (uint32_t i = 0; i < CHD_MAX; ++i)
		delete this->_children[i];
}

Octree &
Octree::operator=(Octree const &rhs)
{
	if (this != &rhs)
	{
		this->_state = rhs.getState();
		this->_cube = rhs.getCube();
	}
	return (*this);
}

int
Octree::subdivide(void)
{
	for (uint32_t i = 0; i < CHD_MAX; ++i)
		this->createChild(i);
	return (0);
}

// NULL check and no given coordinates
int
Octree::createChild(uint32_t const &i)
{
	if (this->_children[i] == NULL)
	{
		float const     s = this->_cube.getS() / 2.0f;

		this->_children[i] = new Octree(this->_cube.getX() + ((i >> 0) & MASK_1) * s,
										this->_cube.getY() + ((i >> 1) & MASK_1) * s,
										this->_cube.getZ() + ((i >> 2) & MASK_1) * s,
										s);
		this->_children[i]->setParent(this);
		return (1);
	}
	return (0);
}

void
Octree::grow(uint32_t const &gd) // gd : grow direction [0, 1, 2, 3, 4, 5, 6, 7]
{
	if (this->_parent != NULL)
		return ;
	this->ground_depth++;
	this->chunk_depth++;
	this->_parent = new Octree( this->_cube.getX() - (~(gd >> 0) & MASK_1)  * this->_cube.getS(),
								this->_cube.getY() - (~(gd >> 1) & MASK_1)  * this->_cube.getS(),
								this->_cube.getZ() - (~(gd >> 2) & MASK_1)  * this->_cube.getS(),
								this->_cube.getS()                          * 2);
	this->_parent->setChild(~gd & 3, this);
}

// no NULL check and coordinates given
void
Octree::createChild(uint32_t const &i, float const &x, float const &y, float const &z, float const &s)
{
	this->_children[i] = new Octree(x, y, z, s);
	this->_children[i]->setParent(this);
}

Octree *
Octree::search(float const &x, float const &y, float const &z)
{
	Octree *		child;
	int				i;

	if (this->_cube.vertexInside(x, y, z))
	{
		child = NULL;
		for (i = 0; i < CHD_MAX; ++i)
		{
			if (this->_children[i] != NULL)
			{
				if ((child = this->_children[i]->search(x, y, z)) != NULL)
					break;
			}
		}
		return (child == NULL ? this : child);
	}
	else
		return (NULL);
}

Octree *
Octree::insert(float const &x, float const &y, float const &z, uint32_t const &depth, uint32_t const &state)
{
	// size never changes for children.
	float const     s = this->_cube.getS() / 2.0f;
	float           nx;
	float           ny;
	float           nz;

	if (depth > 0)
	{
		int				i;

		if (depth == this->ground_depth - this->chunk_depth)
			this->setState(CHUNK);
		for (i = 0; i < CHD_MAX; ++i)
		{
			if (this->_children[i] == NULL)
			{
				/*
				** precomputing child coordinates in order to check if the vertex is inside.
				** new_dim = dim + coefficient * size;
				*/
				nx = this->_cube.getX() + ((i >> 0) & MASK_1) * s;
				ny = this->_cube.getY() + ((i >> 1) & MASK_1) * s;
				nz = this->_cube.getZ() + ((i >> 2) & MASK_1) * s;
				if (x >= nx && x < nx + s && y >= ny && y < ny + s && z >= nz && z < nz + s)
				{
					this->createChild(i, nx, ny, nz, s);
					return (this->_children[i]->insert(x, y, z, depth - 1, state));
				}
			}
			else if (this->_children[i]->getCube()->vertexInside(x, y, z))
				return (this->_children[i]->insert(x, y, z, depth - 1, state));
		}
	}
	else if (depth == 0)
	{
		// max depth reached, put values here
		this->setState(state);
		// std::cerr << "insert and returns what i want: " << this << std::endl; 
		return (this);
	}
	// std::cerr << "returns what i don't want: " << 0 << std::endl;
	return (NULL);
}

void
Octree::drawCube(float const &x, float const &y, float const &z, float const &s) const
{
	glBegin(GL_TRIANGLES);
		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x,       y,      z + s); // 4
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x,       y,      z + s); // 4
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x,       y,      z + s); // 4
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x,       y,      z + s); // 4
	glEnd();
}

void
Octree::drawCubeRidges(float const &x, float const &y, float const &z, float const &s) const
{
	glBegin(GL_LINES);
		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x,       y,      z);     // 0

		glVertex3f(x,       y,      z);     // 0
		glVertex3f(x,       y,      z + s); // 4
		glVertex3f(x + s,   y,      z);     // 1
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x + s,   y + s,  z);     // 3
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x,       y + s,  z);     // 2
		glVertex3f(x,       y + s,  z + s); // 6

		glVertex3f(x,       y,      z + s); // 4
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x + s,   y,      z + s); // 5
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x + s,   y + s,  z + s); // 7
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x,       y + s,  z + s); // 6
		glVertex3f(x,       y,      z + s); // 4
	glEnd();
}

void
Octree::renderGround(float const &r, float const &g, float const &b) const
{
	float const &	z = this->_cube.getZ();
	float			tmp;

	if (this->_state == GROUND)
	{
		tmp = ((double)random() / (double)RAND_MAX) / 30;
		if (z >= 0.2f)
			glColor3f(0.2f - tmp, 0.5f - tmp, 0.2f);
		else if (z >= 0.0f)
			glColor3f(0.7f - tmp, 0.5f - tmp, 0.2f);
		else if (z <= -0.1f)
			glColor3f(0.4f - tmp, 0.4f - tmp, 0.4f - tmp);
		else if (z <= 0.0f)
			glColor3f(0.5f - tmp, 0.5f - tmp, 0.5f - tmp);
		drawCube(this->_cube.getX(), this->_cube.getY(), z, this->_cube.getS());
		glColor3f(0.0f, 0.0f, 0.0f);
	}
	else if (this->_parent == NULL)
	{
		glColor3f(1.0f, 1.0f, 1.0f);
		drawCubeRidges(this->_cube.getX(), this->_cube.getY(), z, this->_cube.getS());
	}
	else if (this->_state == CHUNK)
	{
		glColor3f(1.0f, 0.5f, 0.0f);
		drawCubeRidges(this->_cube.getX(), this->_cube.getY(), z, this->_cube.getS());
	}
	for (int i = 0; i < CHD_MAX; ++i)
	{
		if (this->_children[i] != NULL)
		{
			this->_children[i]->renderGround(r, g, b);
		}
	}
}

void
Octree::setCube(float const &x, float const &y, float const &z, float const &s)
{
	this->_cube = Cube(x, y, z, s);
}

void
Octree::setState(int32_t const &state)
{
	this->_state = state;
}

void
Octree::setParent(Octree *parent)
{
	this->_parent = parent;
}

void
Octree::setChild(uint32_t const &i, Octree *const child)
{
	this->_children[i] = child;
}

Cube const &
Octree::getCube(void) const
{
	return (this->_cube);
}

Cube *
Octree::getCube(void)
{
	return (&this->_cube);
}

int32_t const &
Octree::getState(void) const
{
	return (this->_state);
}

Octree *
Octree::getChild(uint32_t const &i) const
{
	return (this->_children[i]);
}

Octree *
Octree::getParent(void)
{
	return (this->_parent);
}

uint32_t
Octree::getDepth(void)
{
	Octree          *tmp = this->_parent;
	uint32_t        depth = 0;

	while (tmp != NULL)
	{
		depth++;
		tmp = tmp->getParent();
	}
	return (depth);
}

std::ostream &
operator<<(std::ostream &o, Octree &t)
{
	uint32_t const      depth = t.getDepth();

	if (t.getParent() == &t)
		o << "Octree: ";
	o   << "state = "
		<< t.getState()
		<< ", "
		<< t.getCube();
	for (uint32_t it = 0; it < CHD_MAX; ++it)
	{
		if (t.getChild(it) != NULL)
		{
			o << std::endl;
			for (uint32_t i = 0; i < depth; i++)
				o << "-";
			o   << "- child "
				<< it
				<< ": "
				<< *(t.getChild(it));
		}
	}
	return (o);
}

/*

SUBDIVISION PROCESS

i = 0;
000 -> 0
nx = x + 0 * s;
ny = y + 0 * s;
nz = z + 0 * s;

i = 1;
001 -> 1
nx = x + 1 * s;
ny = y + 0 * s;
nz = z + 0 * s;

i = 2;
010 -> 2
nx = x + 0 * s;
ny = y + 1 * s;
nz = z + 0 * s;

i = 3;
011 -> 3
nx = x + 1 * s;
ny = y + 1 * s;
nz = z + 0 * s;

i = 4;
100 -> 4
nx = x + 0 * s;
ny = y + 0 * s;
nz = z + 1 * s;

i = 5;
101 -> 5
nx = x + 1 * s;
ny = y + 0 * s;
nz = z + 1 * s;

i = 6;
110 -> 6
nx = x + 0 * s;
ny = y + 1 * s;
nz = z + 1 * s;

i = 7;
111 -> 7
nx = x + 1 * s;
ny = y + 1 * s;
nz = z + 1 * s;

get bit:
z -> i & 1
y -> (i >> 1) & 1
x -> (i >> 2) & 1

*/

