/* Renderer for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "renderer.h"

#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_GFX.h>

#include "config.h"
#include "display_utils.h"
#include "icons.h"

Renderer::Renderer(GxEPD2_GFX &display) : _display(display) {}

/*
 * Returns the string width in pixels
 */
uint16_t Renderer::getStringWidth(const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

/*
 * Returns the string height in pixels
 */
uint16_t Renderer::getStringHeight(const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return h;
}

/*
 * Draws a string with alignment
 */
void Renderer::drawString(int16_t x, int16_t y, const String &text,
                          alignment_t alignment) {
  int16_t x1, y1;
  uint16_t w, h;
  _display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT) {
    x = x - w;
  }
  if (alignment == CENTER) {
    x = x - w / 2;
  }
  _display.setCursor(x, y);
  _display.print(text);
  return;
}  // end drawString

/*
 * Draws a string that will flow into the next line when max_width is reached.
 * If a string exceeds max_lines an ellipsis (...) will terminate the last word.
 * Lines will break at spaces(' ') and dashes('-').
 *
 * Returns the number of lines drawn.
 *
 * Note: max_width should be big enough to accommodate the largest word that
 *       will be displayed. If an unbroken string of characters longer than
 *       max_width exist in text, then the string will be printed beyond
 *       max_width.
 */
int16_t Renderer::drawMultiLnString(int16_t x, int16_t y, const String &text,
                                    alignment_t alignment, uint16_t max_width,
                                    uint16_t max_lines, int16_t line_spacing) {
  uint16_t current_line = 0;
  String textRemaining = text;
  // print until we reach max_lines or no more text remains
  while (current_line < max_lines && !textRemaining.isEmpty()) {
    int16_t x1, y1;
    uint16_t w, h;

    _display.getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

    int endIndex = textRemaining.length();
    // check if remaining text is to wide, if it is then print what we can
    String subStr = textRemaining;
    int splitAt = 0;
    int keepLastChar = 0;
    while (w > max_width && splitAt != -1) {
      if (keepLastChar) {
        // if we kept the last character during the last iteration of this while
        // loop, remove it now so we don't get stuck in an infinite loop.
        subStr.remove(subStr.length() - 1);
      }

      // find the last place in the string that we can break it.
      if (current_line < max_lines - 1) {
        splitAt = std::max(subStr.lastIndexOf(" "), subStr.lastIndexOf("-"));
      } else {
        // this is the last line, only break at spaces so we can add ellipsis
        splitAt = subStr.lastIndexOf(" ");
      }

      // if splitAt == -1 then there is an unbroken set of characters that is
      // longer than max_width. Otherwise if splitAt != -1 then we can continue
      // the loop until the string is <= max_width
      if (splitAt != -1) {
        endIndex = splitAt;
        subStr = subStr.substring(0, endIndex + 1);

        char lastChar = subStr.charAt(endIndex);
        if (lastChar == ' ') {
          // remove this char now so it is not counted towards line width
          keepLastChar = 0;
          subStr.remove(endIndex);
          --endIndex;
        } else if (lastChar == '-') {
          // this char will be printed on this line and removed next iteration
          keepLastChar = 1;
        }

        if (current_line < max_lines - 1) {
          // this is not the last line
          _display.getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
        } else {
          // this is the last line, we need to make sure there is space for
          // ellipsis
          _display.getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
          if (w <= max_width) {
            // ellipsis fit, add them to subStr
            subStr = subStr + "...";
          }
        }

      }  // end if (splitAt != -1)
    }  // end inner while

    drawString(x, y + (current_line * line_spacing), subStr, alignment);

    // update textRemaining to no longer include what was printed
    // +1 for exclusive bounds, +1 to get passed space/dash
    textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);

    ++current_line;
  }  // end outer while

  return current_line;
}  // end drawMultiLnString

/* This function is responsible for drawing the status bar along the bottom of
 * the display.
 *
 * Returns the height of the status bar (above the baseline)
 */
int16_t Renderer::drawStatusBar(int16_t yBaseline, time_t lastUpdatedTime,
                                int rssi, uint32_t batPercent) {
  
  Serial.printf("drawtatusbar: yBaseline is %lu", yBaseline);

  String dataStr;
  _display.setFont(&FreeSans9pt7b);
  int pos = _display.width() - X_MARGIN - 8;
  const int16_t sp = 12;
  const int16_t iconSize = 18;

  // Battery
  dataStr = String(batPercent) + "%";
  drawString(pos, yBaseline - 2, dataStr, RIGHT);
  pos -= getStringWidth(dataStr) + iconSize + 2;
  drawBattery(pos, yBaseline - 16, iconSize, iconSize, batPercent);

  // WiFi
  pos -= sp;
  dataStr = String(getWiFiDesc(rssi));
  drawString(pos, yBaseline - 2, dataStr, RIGHT);
  pos -= getStringWidth(dataStr) + iconSize + 2;
  drawWifi(pos, yBaseline - 16, iconSize, iconSize, rssi);

  // Last Refresh
  pos -= sp;
  char lastUpdatedTimeString[48];
  strftime(lastUpdatedTimeString, sizeof(lastUpdatedTimeString), "%d %b %H:%M",
           localtime(&lastUpdatedTime));
  drawString(pos, yBaseline - 2, lastUpdatedTimeString, RIGHT);
  pos -= getStringWidth(lastUpdatedTimeString) + 24;
  _display.drawInvertedBitmap(pos, yBaseline - 20, epd_bitmap_refresh, 24, 24,
                              GxEPD_BLACK);

  return 18;
}  // end drawStatusBar

/* This function is responsible for drawing prominent error messages to the
 * screen.
 *
 * If error message line 2 (errMsgLn2) is empty, line 1 will be automatically
 * wrapped.
 */
void Renderer::drawError(const uint8_t *bitmap_192x192, const String &errMsgLn1,
                         const String &errMsgLn2) {
  const uint16_t iconSize = 192;
  const uint16_t displayWidth = _display.width();
  const uint16_t displayHeight = _display.height();
  _display.setFont(&FreeSans24pt7b);
  if (!errMsgLn2.isEmpty()) {
    drawString(displayWidth / 2, displayHeight / 2 + iconSize / 2 + 21,
               errMsgLn1, CENTER);
    drawString(displayWidth / 2, displayHeight / 2 + iconSize / 2 + 21 + 55,
               errMsgLn2, CENTER);
  } else {
    drawMultiLnString(displayWidth / 2, displayHeight / 2 + iconSize / 2 + 21,
                      errMsgLn1, CENTER, displayWidth - 2 * X_MARGIN, 2, 55);
  }
  _display.drawInvertedBitmap(displayWidth / 2 - iconSize / 2,
                              displayHeight / 2 - iconSize / 2 - 21,
                              bitmap_192x192, iconSize, iconSize, GxEPD_BLACK);
  return;
}  // end drawError

void Renderer::drawBattery(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint32_t batPercent) {
  int16_t strokeWidth = 2;
  int16_t xIndent = w / 24;
  int16_t yIndent = h / 4;
  int16_t batteryThickness = h - 2 * yIndent;
  int16_t bumpIndent = batteryThickness / 4;
  int16_t bumpWidth = batteryThickness - 2 * bumpIndent;
  int16_t bumpLength = w / 8;
  int16_t batteryLength = w - bumpLength - 2 * xIndent;
  // Main Battery
  _display.fillRect(x + xIndent, y + yIndent, batteryLength, batteryThickness,
                    GxEPD_BLACK);
  int16_t emptyLength =
      ceil(((batteryLength - 2 * strokeWidth) * (100 - batPercent)) / 100.0);
  _display.fillRect(x + xIndent + batteryLength - strokeWidth - emptyLength,
                    y + yIndent + strokeWidth, emptyLength,
                    batteryThickness - 2 * strokeWidth, GxEPD_WHITE);

  // Bump
  _display.fillRect(x + xIndent + batteryLength, y + yIndent + bumpIndent,
                    bumpLength, bumpWidth, GxEPD_BLACK);
}

void Renderer::drawArc(int16_t cx, int16_t cy, double startAngle,
                       double endAngle, int16_t innerRadius,
                       int16_t outerRadius) {
  for (int16_t y = -outerRadius; y <= outerRadius; y++) {
    for (int16_t x = -outerRadius; x <= outerRadius; x++) {
      int32_t distSquared = x * x + y * y;
      double angle = atan2(-y, x);
      if (distSquared <= outerRadius * outerRadius &&
          distSquared > innerRadius * innerRadius &&
          isAngleInArc(angle, startAngle, endAngle)) {
        _display.drawPixel(cx + x, cy + y, GxEPD_BLACK);
      }
    }
  }
}

inline boolean Renderer::isAngleInArc(double angle, double startAngle,
                                      double endAngle) {
  if (startAngle > endAngle) {
    return angle > startAngle || angle < endAngle;
  }
  return angle > startAngle && angle < endAngle;
}

void Renderer::drawWifi(int16_t x, int16_t y, int16_t w, int16_t h, int rssi) {
  int16_t xIndent = w / 12;
  int16_t yIndent = h / 8;
  int16_t cx = x + w / 2;
  int16_t cy = y + h - yIndent;

  // Recall that angles are measured anti-clockwise
  double startAngle = atan2(h - 3 * yIndent, w / 2 - xIndent);
  double endAngle = atan2(h - 3 * yIndent, xIndent - w / 2);

  int16_t radius = ceil(hypot(w / 2 - xIndent, h - 3 * yIndent));
  float wifiFrac = getWifiFrac(rssi);
  int16_t wifiRadius = ceil(radius * wifiFrac);

  // Draw outline
  drawArc(cx, cy, startAngle, endAngle, radius - 1, radius + 1);
  _display.drawLine(cx, cy, x + xIndent, y + 2 * yIndent, GxEPD_BLACK);
  _display.drawLine(cx, cy - 1, x + xIndent + 1, y + 2 * yIndent, GxEPD_BLACK);
  _display.drawLine(cx, cy, x + w - xIndent, y + 2 * yIndent, GxEPD_BLACK);
  _display.drawLine(cx, cy - 1, x + w - xIndent - 1, y + 2 * yIndent,
                    GxEPD_BLACK);

  // Draw wifi strength
  drawArc(cx, cy, startAngle, endAngle, 0, wifiRadius);
}