#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "gltf.h"

#pragma comment(lib, "opengl32")

int mx = 0, my = 0;
float camx = 10, camy = 0;
float aa = -50;

struct file_mapping
{
	HANDLE file;
	DWORD size;
	HANDLE mapping = 0;
	const unsigned char* base = 0;
	file_mapping(const char* file_name)
	{
		file = CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		if (file != INVALID_HANDLE_VALUE)
		{
			size = GetFileSize(file, 0);
			mapping = CreateFileMapping(file, 0, FILE_MAP_READ, 0, size, 0);
			if (mapping)
				base = (const unsigned char*)MapViewOfFile(mapping, PAGE_READONLY, 0, 0, size);
		}
		else
		{
			char message[1024];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, message, 1024, 0);
			printf("Error: %s\n", message);
		}
	}
	operator const void* ()
	{
		return base;
	}
	operator const unsigned char* ()
	{
		return base;
	}
	~file_mapping()
	{
		if (base)
			UnmapViewOfFile(base);
		if (mapping)
			CloseHandle(mapping);
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}
};
int width, height;
bool keys[4] = { 0 };
LRESULT wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(wnd);
		PostQuitMessage(0);
		break;
	case WM_MOUSEMOVE:
	{
		int _mx = lParam & 0xFFFF;
		int _my = lParam >> 16;

		SetCursorPos(100, 100);
		mx += _mx - 100;
		my += _my - 100;

		mx = max(mx, 0);
		my = max(my, 0);

		mx = min(mx, width);
		my = min(my, height);
	}
	break;
	case WM_KEYDOWN:
	{
		printf("Keydown: %X %X\n", wParam, lParam);
		int k = wParam - 0x25;
		if (k >= 0 && k < 4)
			keys[k] = true;
	}
	break;
	case WM_KEYUP:
	{
		int k = wParam - 0x25;
		if (k >= 0 && k < 4)
			keys[k] = false;
	}
	break;
	}
	return CallWindowProc(DefWindowProc, wnd, msg, wParam, lParam);
}

double Time()
{
	static struct freq_t
	{
		double freq;
		freq_t()
		{
			uint64_t ifreq;
			QueryPerformanceFrequency((LARGE_INTEGER*)&ifreq);
			freq = (double)ifreq;
		}
	} freq;
	uint64_t time;
	QueryPerformanceCounter((LARGE_INTEGER*)&time);
	return (double)time / (double)freq.freq;
}

const double m_1sqrt3 = sqrt(3.)/2.;

int sx, sy;
#include <vector>
struct entity
{
	int id;
	entity(const entity& other) :id(other.id) {}
	entity(int id) :id(id) {}
	entity()
	{
		id = 0xFFFFFFFF;
	}
};

struct tile_idx
{
	int x;
	int y;
};

struct tile_collection
{
	std::map<int, std::vector<int> > content;
	struct iterator
	{
		const tile_collection& parent;
		std::map<int, std::vector<int> >::iterator i1;
		int i2;
		iterator &operator++()
		{
			++i2;
			if (i2 >= i1->second.size())
			{
				++i1;
				i2 = 0;
			}
			return *this;
		}
		tile_idx operator*()
		{
			return { i1->first, i1->second[i2] };
		}
		iterator(const tile_collection& parent) :parent(parent)
		{
		}
		bool operator != (const iterator& other)
		{
			return i1 != other.i1 || i2 != other.i2;
		}
	};
	iterator begin()
	{
		iterator tmp(*this);
		tmp.i1 = content.begin();
		tmp.i2 = 0;
		return tmp;
	}
	iterator end()
	{
		iterator tmp(*this);
		tmp.i1 = content.end();
		tmp.i2 = 0;
		return tmp;
	}
	void add(const tile_idx& t)
	{
		if (content.find(t.x) == content.end())
		{
			content[t.x] = std::vector<int>();
			content[t.x].push_back(t.y);
		}
		else if (std::find(content[t.x].begin(), content[t.x].end(), t.y) == content[t.x].end())
			content[t.x].push_back(t.y);
	}
	bool is_in(const tile_idx& t)
	{
		if (content.find(t.x) != content.end())
			if (std::find(content[t.x].begin(), content[t.x].end(), t.y) != content[t.x].end())
				return true;
		return false;
	}
};

std::vector<entity> entity_map;

unsigned int hash(unsigned int x, unsigned int seed); // 24 bits
unsigned int hash2(unsigned int x, unsigned int y, unsigned int seed)
{
	return hash(hash(x+y, seed) ^ hash(x, seed), seed<<1);
}

tile_collection GetNeighbors(int x, int y)
{
	tile_collection ret;
	
	for (int cx = x - 1; cx <= x + 1; cx++)
		for (int cy = y - 1; cy <= y + 1; cy++)
			if (x == cx && y == cy)
				;
			else if (abs(x - cx) == 1 && y == cy)
				ret.add({ cx,cy });
			else if (abs(y - cy) == 1 && x == cx - (~cy & 1))
				ret.add({ cx, cy });
			else if (abs(y - cy) == 1 && x == cx + (cy & 1))
				ret.add({ cx, cy });

	return ret;
}

gltf model(file_mapping("..\\hex.glb"));
GLuint hex_tex[16];
void DrawScene(bool high_quality, int y_off = 0)
{
	if (high_quality)
	{
		glEnable(GL_TEXTURE_2D);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	int seed = 42;
	int count = 0;
	tile_collection nn = GetNeighbors(sx, sy);
	for (int x = -4; x <= 4; x++)
		for (int y = 0; y < 30; y++)
		{
			count++;
			glPushName(entity_map.size());
			entity_map.push_back(entity((x << 16) | y));
			glPushMatrix();
			glTranslated(m_1sqrt3 * (2*x + (y&1) - 0.5), 1.5 * y, 0);
			glScalef(1, 1, -1);
			unsigned char q = hash2(x+6, y_off + y+6, seed);
			
			bool is_in = nn.is_in({ x,y });
			if (x == sx && y == sy)
				glBindTexture(GL_TEXTURE_2D, hex_tex[15]);
			else if (is_in)
				glBindTexture(GL_TEXTURE_2D, hex_tex[1]);
			else
				glBindTexture(GL_TEXTURE_2D, hex_tex[4]);


			model.draw(q&2?"Tree":q&1?"Plant":"Soil");

			glPopMatrix();
			glPopName();
		}

}

unsigned int hash(unsigned int x, unsigned int seed) // 24 bits
{
	const unsigned int fi = 2654435769; // 1/fi * 2^23
	return (((x * fi * (x^seed)))>>8);
}

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

void generateViewMatrix(float x1, float x2, float h, float m[4][4])
{
	float d1 = sqrtf(x1 * x1 + h * h);
	float d2 = sqrtf(x2 * x2 + h * h);

	float qx = (d1 * x2 + d2 * x1) / (d1 + d2);

	float a = qx;
	float b = -h;
	float det = a * a + b * b;
	float c = sqrtf(a * a / det);
	float s = -sqrtf(b * b / det);
	memset(m, 0, 4 * 4 * sizeof(float));
	
	m[0][0] = 1;
	
	m[2][1] = c;
	m[2][3] = s;
	
	m[1][1] = -s;
	m[1][3] = c;
	
	m[3][0] = 0;
	m[3][1] = -c * h;
	m[3][3] = -s * h;
	m[3][2] = 1;

	float f = -(m[1][3] * x1 + m[3][3]) / (m[1][1] * x1 + m[3][1]);
	for (int cx = 0; cx < 2; cx++)
		for (int cy = 0; cy < 4; cy++)
			m[cy][cx] *= f;

}

int main(void)
{
	DEVMODE dev_mode;
	const char *fixed[3];
	fixed[DMDFO_DEFAULT] = "default";
	fixed[DMDFO_CENTER] = "center";
	fixed[DMDFO_STRETCH] = "stretch";
	
	int width = 800;
	int height = 600;

	for (int cx = 0; EnumDisplaySettings(0, cx, &dev_mode); cx++)
	{

		printf("%S %ix%i@%iHz %s // %hi\n", dev_mode.dmDeviceName, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency, fixed[dev_mode.dmDisplayFixedOutput], dev_mode.dmYResolution);
		if (dev_mode.dmPelsWidth == width && dev_mode.dmPelsHeight == height)
		{
			::width = width;
			::height = height;
//			ChangeDisplaySettings(&dev_mode, CDS_FULLSCREEN);
			break;
		}
	}
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = wndProc;
	wc.lpszClassName = L"MainCharacter";
	wc.hCursor = 0;
	RegisterClass(&wc);
	
	HWND wnd = CreateWindow(wc.lpszClassName, L"MainCharacter", 0, 0, 0, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, 0, 0, 0, 0);
	SetWindowLong(wnd, GWL_STYLE, 0);
	SetWindowPos(wnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
	

	HDC dc = GetDC(wnd);
	PIXELFORMATDESCRIPTOR pfd;
	for (int cx = 1; DescribePixelFormat(dc, cx, sizeof(pfd), &pfd); cx++)
	{
		if (pfd.dwFlags & PFD_SUPPORT_OPENGL && pfd.dwFlags & PFD_DOUBLEBUFFER)
		{
			printf("R:%hhu G:%hhu B:%hhu D:%hhu S:%hhu\n", pfd.cRedBits, pfd.cBlueBits, pfd.cGreenBits, pfd.cDepthBits, pfd.cStencilBits);
			if (pfd.cRedBits == 8 && pfd.cGreenBits == 8 && pfd.cBlueBits == 8 && pfd.cStencilBits && pfd.cDepthBits >= 24)
			{
				SetPixelFormat(dc, cx, &pfd);
				break;
			}
		}
	}
	HGLRC glrc = wglCreateContext(dc);
	wglMakeCurrent(dc, glrc);
	SetFocus(wnd);

	ShowCursor(FALSE);

	double last_time = Time();

	printf("%s\n", glGetString(GL_VENDOR));

	glGenTextures(16, hex_tex);
	unsigned long hex;
	for (int cx = 0; cx < 16; cx++)
	{
		glBindTexture(GL_TEXTURE_2D, hex_tex[cx]);
		hex = cx & 1 ? 0xFF:0;
		hex |= cx & 2 ? 0xFF00 : 0;
		hex |= cx & 4 ? 0xFF00FF : 0;
		hex |= cx & 8 ? 0x808080 : 0;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,1,1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &hex);
	}
	float y = 0;
	double next_frame = Time() + 0.03;
	
	glDepthFunc(GL_GREATER);
	glClearDepth(0);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	GLfloat light_pos[4] = { 1,1,1,0 };
	GLfloat ambient[4] = { 0.3f, 0.8f, 0.5f, 1 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

	double next_world_step = Time() + 1;

	while (1)
	{
		if (Time() > next_world_step)
		{
			next_world_step += 1;
		}
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, 1))
		{
			if (msg.message == WM_QUIT)
				goto hell;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		double now = Time();
//		printf("%f\n", 1. / (now - last_time));
		glEnable(GL_TEXTURE_2D);
		glClearColor(0,0,0, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		GLuint selectBuffer[1024];
		glSelectBuffer(1024, selectBuffer);
		glRenderMode(GL_SELECT);
		glInitNames();

		entity_map.clear();
		glEnable(GL_DEPTH_TEST);

		for (int pass = 0; pass < 2; pass++)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			if (!pass)
			{ // Select projection for one pixel
				glViewport(0, 0, 1, 1);
				glScaled(width / 2., height / 2., 1);
				glTranslated(-(2.*mx - width) / width, -(height - 2.*my) / height, 0);
			}
			else
			{
				glViewport(0, 0, width, height);
			}

// Setup projection
			float m[4][4];
			generateViewMatrix(camy, camy+20, camx, m);
			glMultMatrixf((GLfloat *)m);
			double y_off;
			glTranslated(0, modf(0,&y_off)*2 * 1.5, 0);
			DrawScene(pass, -2 * (int)y_off);
			GLuint s = glRenderMode(GL_RENDER);
			if (s&&s!=-1)
			{
				int ptr = 0;
				int zmin = 0xFFFFFFFF;
				for (int idx = 0; idx < s; idx++)
				{
					int count = selectBuffer[ptr++];
					int z0 = selectBuffer[ptr++];
					int z1 = selectBuffer[ptr++];
					if (count && z0 <= zmin)
					{
						zmin = z0;
						int id = selectBuffer[ptr];
						sx = entity_map[id].id >> 16;
						sy = entity_map[id].id & 0xFFFF;
					}
					ptr += count;
				}
			//	printf("%i: %i %i\n", s, sx, sy);
			}
		}

		glDisable(GL_LIGHTING);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPointSize(5);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		glColor3d(1, 0, 1);
		glBegin(GL_POINTS);
		glVertex2d(-1 + 2. * mx / width, 1 - 2. * my / height);
		glEnd();
		glEnable(GL_LIGHTING);

		last_time = now;
		SwapBuffers(dc);
		next_frame += 0.03;
		if (next_frame > now)
			Sleep(1000 * (next_frame - now));
		if (keys[0])
			camx -= 0.1f;
		if (keys[1])
			camy += 0.1f;
		if (keys[2])
			camx += 0.1f;
		if (keys[3])
			camy -= 0.1f;
		camy = max(0, camy);
		camx = max(0, camx);
		//printf("%f %f\n", camx, camy);
	}
	hell:
	return 0;
}
