#include "SDL3/SDL.h"
#include "simdjson.h"
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <deque>

Uint64 oldTime = 0;
float SCROLL_SPEED = 1;
SDL_Texture* strum = NULL;
SDL_Texture* noteImgs[4] = {NULL};
int16_t angles[] = {0, -90, 90, 180};
SDL_FPoint point = {.x = 52, .y = 52};
SDL_FRect pos = {.x = 0, .y = 0, .w = 104, .h = 104};
Uint16 x[] = {92, 204, 316, 428, 728, 840, 952, 1064};

struct Note {float position; Uint8 x;} __attribute__((packed));
std::deque<Note> notes = {};
std::deque<Uint32> fps = {};

bool exists(std::string fp, bool alert = true) {
	FILE* f = fopen(fp.c_str(), "r");
	if (!f) {
		if (alert) printf("%s does not exist!\n", fp.c_str());
		return false;
	}
	fclose(f);
	return true;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("usage: %s <path to dir with inst.ogg and charts> (speed - no input for default)\n", argv[0]);
		return 0;
	} else if (argc == 3) {
		float n = strtof(argv[2], NULL);
		if (n > 0) {
			SCROLL_SPEED = n;
		} else {
			printf("Invalid Speed: %s - Using Default (1)\n", argv[2]);
		}
	}

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_CreateWindowAndRenderer("FNF' SDL Showcase", 1280, 720, SDL_WINDOW_VULKAN, &window, &renderer);
	//SDL_SetRenderVSync(renderer, 1);

	for (int i = 0; i < 4; i++) {
		char png[14];
		snprintf(png, 14, "assets/%d.png", i);
		SDL_Surface* s = SDL_LoadPNG(png);
		noteImgs[i] = SDL_CreateTextureFromSurface(renderer, s);
		SDL_DestroySurface(s);
	} {
		SDL_Surface* s = SDL_LoadPNG("assets/arrow.png");
		strum = SDL_CreateTextureFromSurface(renderer, s);
		SDL_DestroySurface(s);

		std::string p = argv[1];
		if (!exists(p + "/Inst.wav") && !exists(p + "/Chart.json")) {
			return 1;
		}

		try {
			int i = 0, index = 1;
			simdjson::ondemand::parser parser;
			std::string _p = p + "/Chart";
			p = _p;
			while (exists(p + ".json", false)) {
				printf("%s.json\n", p.c_str());
				auto json = simdjson::padded_string::load(p + ".json");
				auto song = parser.iterate(json);
				for (auto section : (song["song"].type() == simdjson::ondemand::json_type::string ? song["notes"] : song["song"]["notes"])) {
					Uint8 hit = section["mustHitSection"].get_bool() * 4;
					for (auto note : section["sectionNotes"]) {
						simdjson::ondemand::array arr = note.get_array();
						notes.push_back(Note{
							.position = float(arr.at(0).get_double()),
							.x = Uint8(arr.at(1).get_int32() + hit) % 8
						});
					}

					if (i++ % 50 == 0) {
						printf("LOADED SECTION %d (%zu TOTAL NOTES)\n", i, notes.size());
					}
				}

				p = _p + "-" + std::to_string(index++);
				printf("Total Loaded: %d (%zu notes)\n", i, notes.size());
			}
		} catch (const simdjson::simdjson_error& e) {
			printf("ERROR! %s\n", e.what());
			return 1;
		}

		std::sort(notes.begin(), notes.end(), [](Note& a, Note& b) {
			return a.position < b.position; 
		});

		printf("Starting in 5 seconds.\n");
		oldTime = SDL_GetTicks();

		char path[256];
		snprintf(path, 256, "%s/Inst.wav", argv[1]);

		// stupid but it works
		Uint32 len;
		Uint8* songData;
		SDL_AudioSpec spec = {.channels = 2, .freq = 44100};
		SDL_LoadWAV(path, &spec, &songData, &len);

		SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
		SDL_ResumeAudioStreamDevice(stream);
		SDL_PutAudioStreamData(stream, songData, len);
		printf("DONE IN %lu MS!!!\n", oldTime = SDL_GetTicks());
	}

	int total = notes.size();
	while (SDL_RenderPresent(renderer)) {
		SDL_Event e;
		if (SDL_PollEvent(&e) && e.type == SDL_EVENT_QUIT) {
			break;
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		pos.y = 50;
		for (Uint8 i = 0; i < 8; i++) {
			pos.x = x[i];
			SDL_RenderTextureRotated(renderer, strum, NULL, &pos, angles[i % 4], &point, SDL_FLIP_NONE);
		}

		int i = 0, time = SDL_GetTicks() - oldTime, _y = -1;
		bool draws[8] = {0};
		for (Note& note : notes) {
			int y = (note.position - time) * SCROLL_SPEED + 50;
			if (y < 50) {
				notes.pop_front();
				continue;
			} else if (y > 720) {
				break;
			} else if (y == _y && draws[note.x]) {
				i++;
				continue;
			} else if (y != _y) {
				memset(draws, 0, 8);
			}

			i++;
			_y = pos.y = y;
			pos.x = x[note.x];
			draws[note.x] = true;
			SDL_RenderTexture(renderer, noteImgs[note.x % 4], NULL, &pos);
		}

		for (Uint32 f : fps) {
			if (time - f > 1000) {
				fps.pop_front();
				continue;
			}
			break;
		}
		fps.push_back(time);

		SDL_RenderDebugTextFormat(renderer, 8, 16, "FPS: %zu", fps.size());
		SDL_RenderDebugTextFormat(renderer, 8, 8, "Rendered Notes: %d", i);
		SDL_RenderDebugTextFormat(renderer, 8, 698, "%zu/%d", (size_t)total - notes.size(), total);
		SDL_RenderDebugText(renderer, 8, 706, "Nael2xd");
	}

	return 0;
}
