#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "gltf.h"

#pragma comment(lib, "opengl32")

int mx = 0, my = 0;

LRESULT wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
	{
		int _mx = lParam & 0xFFFF;
		int _my = lParam >> 16;

		SetCursorPos(100, 100);
		mx += _mx - 100;
		my += _my - 100;
		
		printf("m: %i %i\n", mx, my);
	}
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

const double hex_x[7] = { 1,0.5,-0.5,-1,-0.5,0.5 ,1};
const double hex_y[7] = { 0, m_1sqrt3,m_1sqrt3,0,-m_1sqrt3,-m_1sqrt3,0 };

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

std::vector<entity> entity_map;

unsigned int hash(unsigned int x, unsigned int seed); // 24 bits
unsigned int hash2(unsigned int x, unsigned int y, unsigned int seed)
{
	return hash(hash(x+y, seed) ^ hash(x, seed), seed<<1);
}
void DrawScene(bool high_quality)
{
	if (high_quality)
	{
		glEnable(GL_TEXTURE_1D);
	}
	else
	{
		glDisable(GL_TEXTURE_1D);
	}
	glScaled(0.1, 0.1, 1);
	int seed = Time();
	for (int x = 0; x < 20; x++)
		for (int y = 0; y < 20; y++)
		{
			glPushName(entity_map.size());
			entity_map.push_back(entity((x << 16) | y));
			glPushMatrix();
			glTranslated(1.5 * x - 1.5 * y, m_1sqrt3 * (x + y), 0);
			unsigned char q = hash2(x+6, y+6, seed);
			if (x == sx && y == sy)
				glColor3d(1, 0, 0);
			else
				glColor3d((q&1)+0.5, ((q>>1)&1)+0.5, ((q>>2)&1)+0.5);
			glBegin(GL_TRIANGLE_FAN);
			glTexCoord1d(0);
			glVertex2d(0, 0);
			glTexCoord1d(1);
			for (int cx = 0; cx <= 6; cx++)
				glVertex2d(hex_x[cx], hex_y[cx]);
			glEnd();
			glPopMatrix();
			glPopName();
		}

}

unsigned int hash(unsigned int x, unsigned int seed) // 24 bits
{
	const unsigned int fi = 2654435769; // 1/fi * 2^23
	return (((x * fi * x))>>8)^seed;
}

struct file_mapping
{
	HANDLE file;
	DWORD size;
	HANDLE mapping = 0;
	const unsigned char* base = 0;
	file_mapping(const char* file_name)
	{
		file = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
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

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main(void)
{
	
	{
		file_mapping m("..\\hex.glb");
		gltf model(m);


		printf("Read\n");
	}
	/*
	int histo[14] = { 0 };
	int follow[14][14] = { 0 };
	int last = 0;
	for (int cy = 0; cy < 500; cy ++)
		for (unsigned int cx = 0; cx < 500; cx++)
		{
			int v = (14 * (hash(hash(cx, 12) ^ hash(cy,12), 12))) >> 24;
			histo[v]++;
			follow[last][v]++;
			last = v;
		}
	for (int cx = 0; cx < 14; cx++)
		printf("%i\n", histo[cx]);
	for (int cy = 0; cy < 14; cy++)
	{
		for (int cx = 0; cx < 14; cx++)
		{
			printf(" %i ", follow[cx][cy]);
		}
		printf("\n");
	}

	*/
	DEVMODE dev_mode;
	const char *fixed[3];
	fixed[DMDFO_DEFAULT] = "default";
	fixed[DMDFO_CENTER] = "center";
	fixed[DMDFO_STRETCH] = "stretch";
	
	for (int cx = 0; EnumDisplaySettings(0, cx, &dev_mode); cx++)
	{

		printf("%s %ix%i@%iHz %s // %hi\n", dev_mode.dmDeviceName, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency, fixed[dev_mode.dmDisplayFixedOutput], dev_mode.dmYResolution);
		if (dev_mode.dmPelsWidth == 800 && dev_mode.dmPelsHeight == 600)
		{
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

	GLuint hex_tex;
	glGenTextures(1, &hex_tex);
	glBindTexture(GL_TEXTURE_1D, hex_tex);
	unsigned char hex[16];
	for (int cx = 0; cx < 16; cx++)
		hex[cx] = cx*cx;
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, hex);

	double next_frame = Time() + 0.03;
	while (1)
	{
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, 1))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		double now = Time();
//		printf("%f\n", 1. / (now - last_time));
		glEnable(GL_TEXTURE_1D);
		glClearColor(sin(now), cos(now), -sin(now), 1);
		glClear(GL_COLOR_BUFFER_BIT);
		GLuint selectBuffer[1024];
		glSelectBuffer(1024, selectBuffer);
		glRenderMode(GL_SELECT);
		glInitNames();

		entity_map.clear();

		for (int pass = 0; pass < 2; pass++)
		{
			glLoadIdentity();

			if (!pass)
			{ // Select projection for one pixel
				glScaled(512, 384, 1);
				glTranslated(-(mx - 512.) / 512., -(384. - my) / 384., 0);
			}
			else
			{
			}
			DrawScene(pass);
			GLuint s = glRenderMode(GL_RENDER);
			glEnable(GL_TEXTURE_1D);
			if (s)
			{
				int ptr = 0;
				for (int idx = 0; idx < s; idx++)
				{
					int count = selectBuffer[ptr++];
					int z0 = selectBuffer[ptr++];
					int z1 = selectBuffer[ptr++];
					if (count)
					{
						int id = selectBuffer[ptr];
						sx = entity_map[id].id >> 16;
						sy = entity_map[id].id & 0xFFFF;
					}
					ptr += count;
				}
				printf("%i: %i %i\n", s, sx, sy);
			}
		}
		glLoadIdentity();
		glPointSize(5);
		glDisable(GL_TEXTURE_1D);
		glColor3d(1, 1, 1);
		glBegin(GL_POINTS);
		glVertex2d(-1 + 2. * mx / 1024., 1 - 2. * my / 768.);
		glEnd();

		last_time = now;
		SwapBuffers(dc);
		next_frame += 0.03;
		if (next_frame > now)
			Sleep(1000 * (next_frame - now));
	}
	return 0;
}
