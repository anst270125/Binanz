
# Binanz
Binanz is a basic price evolution tracker for Binance.com, written in C++ with the help of the Qt framework. It uses the Binance public REST API with *NONE* security level, so no API key or login data are needed. The one and only endpoint used is */api/v3/klines*. I made this tool because many times I wished to visually compare the price evolution of two or more crypto pairs, and couldn't find any tool to do so. Probably I haven't looked good enough, but it was also time to refresh my programming knowledge.

## Features 
  - Download price history for any pair on Binance.com.
  - Inspect the price history for one or more pairs, zooming on any time frame
  - Visualize a chart of showing the price change (%) for one or more pairs, starting at any selected date
  - Visualize a table showing the price change for typical intervals : 24h, 1w, 1m, 3m and 1y.
  - Create a watchlist to quickly load favorite pairs at startup
  - Stumble upon various bugs

![](https://i.imgur.com/J33Gmts.png)

![](https://i.imgur.com/zYvYuUP.png)


### Installation

Either download the release package and run binanz.exe, or clone the repo and compile the code yourself, using qmake and a C++ compiler.

### User guide
**Download data**: To download data for any pair, choose any combination from the two dropdowns, edit the text in the first drop down, or enter any pair in the line edit below, and press Add. The downloaded pairs are added in the list on the right side. To show or hide different pairs on the chart, check or uncheck the checkbox next to the pairs name. The interval of the price points is 4 hours. That means, the 24 hours price difference might not be too accurate.

**Inspect price history**: By default ("*None*" radio button), the chart overlays the price history of all shown pairs. For obvious reasons, this is not very useful for viewing the price history for more than one pair. To zoom into a specific time frame either click on the 1d/7d/1m/3m/1y buttons, or select the desired time frame with the mouse. Click right for zooming out.

**Inspect price evolution**: Toggle the "*% change from date*" radio button in order to compare the price change  of the shown pairs, expressed as a percentage. When this option is enabled, the inspection interval is set to the last month. The price of all pairs will be tied to 0% at that specific moment. In order to change the initial (0%) date, double click on any date from the calendar. The visualized time frame will now zoom to your selection, but further zooming in/out is possible as explained above. Clicking on the 1d/7d/1m/3m/1y buttons in this mode will also change the initial (0%) date accordingly.

**Watchlist**:
Edit watchlist.txt to enter your favorite pairs. Upon startup, the app loads the data for the watched pairs automatically.

**Save data**:
When downloading data for a pair for the first time, the data is stored into the /data folder. Upon subsequent runs of the app, the old data is loaded from the disk, and only the new, missing data is downloaded from binance. This makes the app faster.