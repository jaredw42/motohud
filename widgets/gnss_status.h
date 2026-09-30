#pragma once

#include <stdint.h>

#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QTableWidget>
#include <QWidget>

#include "devices/gnss_client.h"
#include "tile.h"
class GnssStatus : public QWidget {
  Q_OBJECT

public:
  explicit GnssStatus(QWidget *parent = nullptr);

  void setConnectionEndpoint(const QString &host, uint16_t port);

  void setDisconnected();
  void updateDisplay(const GnssPvt &s);

signals:
  void connectionRequested(const QString &host, uint16_t port);

private:
  void buildUi();
  void applyConnectionSettings();

private:
  QLabel *label_ = nullptr;
  Tile *sv_tile_ = nullptr;
  Tile *fix_status_ = nullptr;
  Tile *correction_age_ = nullptr;
  Tile *accuracy_ = nullptr;
  QTableWidget *fix_table_ = nullptr;
  QTableWidget *baseline_ = nullptr;
  QLineEdit *ip_address_input_ = nullptr;
  QLineEdit *port_input_ = nullptr;
  QString connection_host_;
  uint16_t connection_port_ = 8100;

  DynamicTileConfig sv_tile_config_{24, 12, false};

  static constexpr auto kTableStyleSheet = (R"(
  QTableWidget {
    color: white;
    background-color: black;
    gridline-color: gray;
  }
  QHeaderView::section {
    color: white;
    background-color: dimgray;
  }
  QTableWidget::item:selected {
    color: black;
    background-color: lightgreen;
  }
)");

  QTableWidgetItem *svs_used_for_nav_value_ = nullptr;
  QTableWidgetItem *fix_value_ = nullptr;
  QTableWidgetItem *correction_age_value_ = nullptr;
  QTableWidgetItem *estimated_position_accuracy_value_ = nullptr;
  QTableWidgetItem *estimated_speed_accuracy_value_ = nullptr;

  QTableWidgetItem *baseline_distance_n_ = nullptr;
  QTableWidgetItem *baseline_distance_e_ = nullptr;
  QTableWidgetItem *baseline_distance_d_ = nullptr;
  QTableWidgetItem *baseline_distance_2d_ = nullptr;
  QTableWidgetItem *baseline_distance_3d_ = nullptr;

  QTableWidgetItem *velocity_n_ = nullptr;
  QTableWidgetItem *velocity_e_ = nullptr;
  QTableWidgetItem *velocity_d_ = nullptr;
  QTableWidgetItem *velocity_2d_ = nullptr;
  QTableWidgetItem *velocity_3d_ = nullptr;
};