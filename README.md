# Binanz
Binanz is a basic price evolution tracker for Binance.com, written in C++ with the help of the Qt framework. It uses the Binance public REST API with *NONE* security level, so no API key or login data are needed. The one and only endpoint used is */api/v3/klines*. I made this tool because many times I wished to visually compare the price evolution of two or more crypto pairs, and couldn't find any tool to do so. Probably I haven't looked good enough, but it was also time to refresh my programming knowledge.

## Features 
  - Download price history for any pair on Binance.com, using any interval(resolution), like 1h, 4h or 12h.
  - Inspect the price history for one or more pairs, zooming on any time frame
  - Visualize the price change (%) for one or more pairs, starting at any selected date
  - Stumble upon various bugs

![](https://imgur.com/Lf1XlKI.jpg)

![](https://imgur.com/k60gqr2.jpg)


### Installation

Either download the release package and run binanz.exe, or clone the repo and compile the code yourself, using qmake and a C++ compiler.

### User guide

**Download data**: To download data for any pair, choose any combination from the two dropdowns, edit the text in the first drop down, or enter any pair in the line edit below, and press Add. The default interval for individual price points is 12h. You can change this in the topmost dropdown. A shorter interval will result in longer download times, but will be useful for comparing price changes for shorter time frames. The downloaded pairs are added in the list on the right side. To show or hide different pairs on the chart, check or uncheck the checkbox next to the pairs name.

**Inspect price history**: By default ("*None*" radio button), the chart overlays the price history of all shown pairs. For obvious reasons, this is not very useful for viewing the price history for more than one pair. To zoom into a specific time frame either click on the 1d/7d/1m/3m/1y buttons, or select the desired time frame with the mouse. Click right for zooming out.

**Inspect price evolution**: Toggle the "*% change from date*" radio button in order to compare the price change  of the shown pairs, expressed as a percentage. When this option is enabled, the inspection interval is set to the last month. The price of all pairs will be tied to 0% at that specific moment. In order to change the initial (0%) date, double click on any date from the calendar. The visualized time frame will now zoom to your selection, but further zooming in/out is possible as explained above. Clicking on the 1d/7d/1m/3m/1y buttons in this mode will also change the initial (0%) date accordingly.