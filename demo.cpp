#define GL_GLEXT_PROTOTYPES 1
#define NO_SDL_GLEXT 1

#define WIDTH 1280
#define HEIGHT 720

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_image.h>

#include "effect.h"
#include "effect_chain.h"
#include "util.h"
#include "opengl.h"
#include "widgets.h"

#include "flat_input.h"
#include "lift_gamma_gain_effect.h"
#include "saturation_effect.h"
#include "diffusion_effect.h"

unsigned char result[WIDTH * HEIGHT * 4];

float lift_theta = 0.0f, lift_rad = 0.0f, lift_v = 0.0f;
float gamma_theta = 0.0f, gamma_rad = 0.0f, gamma_v = 0.5f;
float gain_theta = 0.0f, gain_rad = 0.0f, gain_v = 0.25f;
float saturation = 1.0f;

//float radius = 0.3f;
// float inner_radius = 0.3f;
float blur_radius = 20.0f;
float blurred_mix_amount = 0.5f;
	
void update_hsv(Effect *lift_gamma_gain_effect, Effect *saturation_effect)
{
	RGBTriplet lift(0.0f, 0.0f, 0.0f);
	RGBTriplet gamma(1.0f, 1.0f, 1.0f);
	RGBTriplet gain(1.0f, 1.0f, 1.0f);

	hsv2rgb_normalized(lift_theta, lift_rad, lift_v, &lift.r, &lift.g, &lift.b);
	hsv2rgb_normalized(gamma_theta, gamma_rad, gamma_v * 2.0f, &gamma.r, &gamma.g, &gamma.b);
	hsv2rgb_normalized(gain_theta, gain_rad, gain_v * 4.0f, &gain.r, &gain.g, &gain.b);

	bool ok = lift_gamma_gain_effect->set_vec3("lift", (float *)&lift);
	ok = ok && lift_gamma_gain_effect->set_vec3("gamma", (float *)&gamma);
	ok = ok && lift_gamma_gain_effect->set_vec3("gain", (float *)&gain);
	assert(ok);

	if (saturation < 0.0) {
		saturation = 0.0;
	}
	ok = saturation_effect->set_float("saturation", saturation);
	assert(ok);
}

void mouse(int x, int y)
{
	float xf = (x / (float)WIDTH) * 16.0f / 9.0f;
	float yf = (HEIGHT - y) / (float)HEIGHT;

	if (yf < 0.2f) {
		read_colorwheel(xf, yf, &lift_rad, &lift_theta, &lift_v);
	} else if (yf >= 0.2f && yf < 0.4f) {
		read_colorwheel(xf, yf - 0.2f, &gamma_rad, &gamma_theta, &gamma_v);
	} else if (yf >= 0.4f && yf < 0.6f) {
		read_colorwheel(xf, yf - 0.4f, &gain_rad, &gain_theta, &gain_v);
	} else if (yf >= 0.6f && yf < 0.62f && xf < 0.2f) {
		saturation = (xf / 0.2f) * 4.0f;
#if 0
	} else if (yf >= 0.65f && yf < 0.67f && xf < 0.2f) {
		radius = (xf / 0.2f);
	} else if (yf >= 0.70f && yf < 0.72f && xf < 0.2f) {
		inner_radius = (xf / 0.2f);
#endif
	} else if (yf >= 0.75f && yf < 0.77f && xf < 0.2f) {
		blur_radius = (xf / 0.2f) * 100.0f;
	} else if (yf >= 0.80f && yf < 0.82f && xf < 0.2f) {
		blurred_mix_amount = (xf / 0.2f);
	}
}

unsigned char *load_image(const char *filename, unsigned *w, unsigned *h)
{
	SDL_Surface *img = IMG_Load(filename);
	if (img == NULL) {
		fprintf(stderr, "Load of '%s' failed\n", filename);
		exit(1);
	}

	SDL_PixelFormat rgba_fmt;
	rgba_fmt.palette = NULL;
	rgba_fmt.BitsPerPixel = 32;
	rgba_fmt.BytesPerPixel = 8;
	rgba_fmt.Rloss = rgba_fmt.Gloss = rgba_fmt.Bloss = rgba_fmt.Aloss = 0;

	// NOTE: Assumes little endian.
	rgba_fmt.Rmask = 0x00ff0000;
	rgba_fmt.Gmask = 0x0000ff00;
	rgba_fmt.Bmask = 0x000000ff;
	rgba_fmt.Amask = 0xff000000;

	rgba_fmt.Rshift = 16;
	rgba_fmt.Gshift = 8;
	rgba_fmt.Bshift = 0;
	rgba_fmt.Ashift = 24;
	
	rgba_fmt.colorkey = 0;
	rgba_fmt.alpha = 255;

	SDL_Surface *converted = SDL_ConvertSurface(img, &rgba_fmt, SDL_SWSURFACE);

	*w = img->w;
	*h = img->h;

	SDL_FreeSurface(img);

	return (unsigned char *)converted->pixels;
}

void write_ppm(const char *filename, unsigned char *screenbuf)
{
	FILE *fp = fopen(filename, "w");
	fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	for (unsigned y = 0; y < HEIGHT; ++y) {
		unsigned char *srcptr = screenbuf + ((HEIGHT - y - 1) * WIDTH) * 4;
		for (unsigned x = 0; x < WIDTH; ++x) {
			fputc(srcptr[x * 4 + 2], fp);
			fputc(srcptr[x * 4 + 1], fp);
			fputc(srcptr[x * 4 + 0], fp);
		}
	}
	fclose(fp);
}

int main(int argc, char **argv)
{
	bool quit = false;

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_OPENGL);
	SDL_WM_SetCaption("OpenGL window", NULL);
	
	// geez	
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	unsigned img_w, img_h;
	unsigned char *src_img = load_image("blg_wheels_woman_1.jpg", &img_w, &img_h);

	EffectChain chain(WIDTH, HEIGHT);

	ImageFormat inout_format;
	inout_format.color_space = COLORSPACE_sRGB;
	inout_format.gamma_curve = GAMMA_sRGB;

	FlatInput *input = new FlatInput(inout_format, FORMAT_BGRA, GL_UNSIGNED_BYTE, img_w, img_h);
	chain.add_input(input);
	Effect *lift_gamma_gain_effect = chain.add_effect(new LiftGammaGainEffect());
	Effect *saturation_effect = chain.add_effect(new SaturationEffect());
	Effect *diffusion_effect = chain.add_effect(new DiffusionEffect());
	//Effect *vignette_effect = chain.add_effect(new VignetteEffect());
	//Effect *sandbox_effect = chain.add_effect(new SandboxEffect());
	//sandbox_effect->set_float("parm", 42.0f);
	//chain.add_effect(new MirrorEffect());
	chain.add_output(inout_format);
	chain.finalize();

	// generate a PBO to hold the data we read back with glReadPixels()
	// (Intel/DRI goes into a slow path if we don't read to PBO)
	GLuint pbo;
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER_ARB, WIDTH * HEIGHT * 4, NULL, GL_STREAM_READ);

	make_hsv_wheel_texture();

	int frame = 0;
	bool screenshot = false;
#if _POSIX_C_SOURCE >= 199309L
	struct timespec start, now;
	clock_gettime(CLOCK_MONOTONIC, &start);
#else
	struct timeval start, now;
	gettimeofday(&start, NULL);
#endif

	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			} else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
				quit = true;
			} else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
				screenshot = true;
			} else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				mouse(event.button.x, event.button.y);
			} else if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(1))) {
				mouse(event.motion.x, event.motion.y);
			}
		}

		++frame;

		update_hsv(lift_gamma_gain_effect, saturation_effect);
		//vignette_effect->set_float("radius", radius);
		//vignette_effect->set_float("inner_radius", inner_radius);
		//vignette_effect->set_vec2("center", (float[]){ 0.7f, 0.5f });

		diffusion_effect->set_float("radius", blur_radius);
		diffusion_effect->set_float("blurred_mix_amount", blurred_mix_amount);

		input->set_pixel_data(src_img);
		chain.render_to_screen();
		
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
		check_error();
		glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, BUFFER_OFFSET(0));
		check_error();
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
		check_error();

		glLoadIdentity();
		draw_hsv_wheel(0.0f, lift_rad, lift_theta, lift_v);
		draw_hsv_wheel(0.2f, gamma_rad, gamma_theta, gamma_v);
		draw_hsv_wheel(0.4f, gain_rad, gain_theta, gain_v);
		draw_saturation_bar(0.6f, saturation / 4.0f);
#if 0
		draw_saturation_bar(0.65f, radius);
		draw_saturation_bar(0.70f, inner_radius);
#endif
		draw_saturation_bar(0.75f, blur_radius / 100.0f);
		draw_saturation_bar(0.80f, blurred_mix_amount);

		SDL_GL_SwapBuffers();
		check_error();

		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
		check_error();
		unsigned char *screenbuf = (unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		check_error();
		if (screenshot) {
			char filename[256];
			sprintf(filename, "frame%05d.ppm", frame);
			write_ppm(filename, screenbuf);
			printf("Screenshot: %s\n", filename);
			screenshot = false;
		}
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		check_error();
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
		check_error();

#if 1
#if _POSIX_C_SOURCE >= 199309L
		clock_gettime(CLOCK_MONOTONIC, &now);
		double elapsed = now.tv_sec - start.tv_sec +
			1e-9 * (now.tv_nsec - start.tv_nsec);
#else
		gettimeofday(&now, NULL);
		double elapsed = now.tv_sec - start.tv_sec +
			1e-6 * (now.tv_usec - start.tv_usec);
#endif
		printf("%d frames in %.3f seconds = %.1f fps (%.1f ms/frame)\n",
			frame, elapsed, frame / elapsed,
			1e3 * elapsed / frame);
#endif
	}
	return 0; 
}
