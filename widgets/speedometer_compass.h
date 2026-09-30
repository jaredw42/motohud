#pragma once

#include <QLabel>
#include <QWidget>

#include "devices/gnss_client.h"
#include "tile.h"

class SpeedometerCompass : public QWidget {
  Q_OBJECT

public:
  explicit SpeedometerCompass(QWidget *parent = nullptr);

  void setDisconnected();
  void updateDisplay(const GnssPvt &gnss_state);
  void resetOdometer();

  void setShowEstimatedSpeedError(bool show);
  void setShowDate(bool show);
  void setShowHeadingDegrees(bool show);
  void setShowSecondaryOdo(bool show);

private:
  void buildUi();

private:
  Tile *speed_tile_ = nullptr;
  Tile *heading_tile_ = nullptr;
  Tile *time_tile_ = nullptr;
  Tile *odo_tile_ = nullptr;
  Tile *sv_tile_ = nullptr;

  DynamicTileConfig speed_tile_cfg_{108, 24, false};
  DynamicTileConfig heading_tile_cfg_{96, 36, true};
  DynamicTileConfig time_tile_cfg_{36, 18, true};
  DynamicTileConfig odo_tile_cfg_{36, 36, false};
  DynamicTileConfig sv_tile_cfg_{32, 24, true};

  QLabel *speed_value_ = nullptr;
  QLabel *estimated_speed_accuracy_value_ = nullptr;
  QLabel *heading_value_ = nullptr;
  QLabel *heading_degrees_value_ = nullptr;
  QLabel *time_value_ = nullptr;
  QLabel *date_value_ = nullptr;
  QLabel *odo_value_ = nullptr;
  QLabel *sv_value_ = nullptr;
  QLabel *fix_value_ = nullptr;

  float start_gnss_odo_val_{112332.2};
  float current_gnss_odo_val_{0.0};
  static constexpr float kMilesToMeters = 1609.34;
  static constexpr float kMillimetersPerSecToMilesPerHour = 0.00223694;
};