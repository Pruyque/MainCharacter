#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#pragma comment(lib, "opengl32")

int mx = 0, my = 0;

LRESULT wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
	{
		mx = lParam & 0xFFFF;
		my = lParam >> 16;
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

	for (int x = 0; x < 5; x++)
		for (int y = 0; y < 5; y++)
		{
			glPushName(entity_map.size());
			entity_map.push_back(entity((x << 16) | y));
			glPushMatrix();
			glTranslated(1.5 * x - 1.5 * y, m_1sqrt3 * (x + y), 0);
			if (x == sx && y == sy)
				glColor3d(1, 0, 0);
			else
				glColor3d(1, 1, 1);
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

unsigned int hash(unsigned int x)
{
	const int fi = 2654435769;
	return x * fi;
}

int main(void)
{
	int histo[14] = { 0 };
	int follow[14][14] = { 0 };
	int last = 0;
	for (int cy = 0; cy < 1000; cy ++)
		for (unsigned int cx = 0; cx < 1000; cx++)
		{
			int v = (14 * (hash(hash(cx) + hash(cy)) & 0xFFFF)) >> 16;
			printf("%i %u -> %u\r", cy, cx, v);
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
	DEVMODE dev_mode;
	const char *fixed[3];
	fixed[DMDFO_DEFAULT] = "default";
	fixed[DMDFO_CENTER] = "center";
	fixed[DMDFO_STRETCH] = "stretch";
	
	for (int cx = 0; EnumDisplaySettings(0, cx, &dev_mode); cx++)
	{
		printf("%ix%i@%iHz %s // %hi\n", dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency, fixed[dev_mode.dmDisplayFixedOutput], dev_mode.dmYResolution);
		if (dev_mode.dmPelsWidth == 1024 && dev_mode.dmPelsHeight == 768)
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
	SetWindowLong(wnd, GWL_STYLE, WS_VISIBLE);
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
