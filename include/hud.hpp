#pragma once
#include <SDL.h>

#include <array>
#include <string>
#include <vector>

/****************************************************************
*	Tiny 3x5 bitmap font, drawn as filled rects. No dependency,
*	no font file, no atlas — matches the pixel aesthetic and
*	renders identically on mac/linux/windows. Used for the
*	on-screen stats readout that replaces the console prints.
****************************************************************/
class Hud {
public:
	// draw a left-aligned block of lines in a translucent panel at (ox, oy)
	void panel(SDL_Renderer* renderer, int ox, int oy, int scale, const std::vector<std::string>& lines) {
		if (lines.empty()) return;
		int longest = 0;
		for (const auto& l : lines) longest = std::max<int>(longest, static_cast<int>(l.size()));
		const int pad = 4 * scale;
		const int line_h = 6 * scale;               // 5 tall + 1 gap
		const int glyph_w = 4 * scale;              // 3 wide + 1 gap
		const int w = pad * 2 + longest * glyph_w;
		const int h = pad * 2 + static_cast<int>(lines.size()) * line_h;

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, 10, 14, 20, 165);
		SDL_Rect bg{ ox, oy, w, h };
		SDL_RenderFillRect(renderer, &bg);

		int y = oy + pad;
		for (const auto& line : lines) {
			draw_text(renderer, ox + pad, y, scale, line, 220, 232, 240);
			y += line_h;
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	}

	void draw_text(SDL_Renderer* renderer, int x, int y, int scale, const std::string& text,
	               Uint8 r, Uint8 g, Uint8 b) {
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		int cx = x;
		for (char c : text) {
			const auto& glyph = glyph_of(c);
			for (int row = 0; row < 5; row++) {
				for (int col = 0; col < 3; col++) {
					if (glyph[row][col] == '#') {
						SDL_Rect px{ cx + col * scale, y + row * scale, scale, scale };
						SDL_RenderFillRect(renderer, &px);
					}
				}
			}
			cx += 4 * scale;
		}
	}

private:
	using Glyph = std::array<const char*, 5>;

	static const Glyph& glyph_of(char c) {
		if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
		switch (c) {
			case '0': { static const Glyph g{ "###","# #","# #","# #","###" }; return g; }
			case '1': { static const Glyph g{ " # ","## "," # "," # ","###" }; return g; }
			case '2': { static const Glyph g{ "###","  #","###","#  ","###" }; return g; }
			case '3': { static const Glyph g{ "###","  #","###","  #","###" }; return g; }
			case '4': { static const Glyph g{ "# #","# #","###","  #","  #" }; return g; }
			case '5': { static const Glyph g{ "###","#  ","###","  #","###" }; return g; }
			case '6': { static const Glyph g{ "###","#  ","###","# #","###" }; return g; }
			case '7': { static const Glyph g{ "###","  #","  #","  #","  #" }; return g; }
			case '8': { static const Glyph g{ "###","# #","###","# #","###" }; return g; }
			case '9': { static const Glyph g{ "###","# #","###","  #","###" }; return g; }
			case 'A': { static const Glyph g{ "###","# #","###","# #","# #" }; return g; }
			case 'B': { static const Glyph g{ "## ","# #","## ","# #","## " }; return g; }
			case 'C': { static const Glyph g{ "###","#  ","#  ","#  ","###" }; return g; }
			case 'D': { static const Glyph g{ "## ","# #","# #","# #","## " }; return g; }
			case 'E': { static const Glyph g{ "###","#  ","## ","#  ","###" }; return g; }
			case 'F': { static const Glyph g{ "###","#  ","## ","#  ","#  " }; return g; }
			case 'G': { static const Glyph g{ "###","#  ","# #","# #","###" }; return g; }
			case 'H': { static const Glyph g{ "# #","# #","###","# #","# #" }; return g; }
			case 'I': { static const Glyph g{ "###"," # "," # "," # ","###" }; return g; }
			case 'J': { static const Glyph g{ "  #","  #","  #","# #","###" }; return g; }
			case 'K': { static const Glyph g{ "# #","# #","## ","# #","# #" }; return g; }
			case 'L': { static const Glyph g{ "#  ","#  ","#  ","#  ","###" }; return g; }
			case 'M': { static const Glyph g{ "# #","###","###","# #","# #" }; return g; }
			case 'N': { static const Glyph g{ "# #","###","###","###","# #" }; return g; }
			case 'O': { static const Glyph g{ "###","# #","# #","# #","###" }; return g; }
			case 'P': { static const Glyph g{ "###","# #","###","#  ","#  " }; return g; }
			case 'Q': { static const Glyph g{ "###","# #","# #","###","  #" }; return g; }
			case 'R': { static const Glyph g{ "###","# #","## ","# #","# #" }; return g; }
			case 'S': { static const Glyph g{ "###","#  ","###","  #","###" }; return g; }
			case 'T': { static const Glyph g{ "###"," # "," # "," # "," # " }; return g; }
			case 'U': { static const Glyph g{ "# #","# #","# #","# #","###" }; return g; }
			case 'V': { static const Glyph g{ "# #","# #","# #","# #"," # " }; return g; }
			case 'W': { static const Glyph g{ "# #","# #","###","###","# #" }; return g; }
			case 'X': { static const Glyph g{ "# #","# #"," # ","# #","# #" }; return g; }
			case 'Y': { static const Glyph g{ "# #","# #","###"," # "," # " }; return g; }
			case 'Z': { static const Glyph g{ "###","  #"," # ","#  ","###" }; return g; }
			case ':': { static const Glyph g{ "   "," # ","   "," # ","   " }; return g; }
			case '.': { static const Glyph g{ "   ","   ","   ","   "," # " }; return g; }
			case ',': { static const Glyph g{ "   ","   ","   "," # ","#  " }; return g; }
			case '/': { static const Glyph g{ "  #","  #"," # ","#  ","#  " }; return g; }
			case '-': { static const Glyph g{ "   ","   ","###","   ","   " }; return g; }
			case '+': { static const Glyph g{ "   "," # ","###"," # ","   " }; return g; }
			case '%': { static const Glyph g{ "# #","  #"," # ","#  ","# #" }; return g; }
			default:  { static const Glyph g{ "   ","   ","   ","   ","   " }; return g; } // space & unknown
		}
	}
};
