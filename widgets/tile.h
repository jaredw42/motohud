// tile.h
#pragma once
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

class Tile final : public QFrame {
  Q_OBJECT
public:
  explicit Tile(const QString &title, QWidget *parent = nullptr)
      : QFrame(parent), title_(new QLabel(title, this)),
        primary_(new QLabel("--", this)), secondary_(new QLabel("", this)) {

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    title_->setObjectName("title");
    primary_->setObjectName("primary");
    secondary_->setObjectName("secondary");

    title_->setAlignment(Qt::AlignCenter);
    primary_->setAlignment(Qt::AlignCenter);
    secondary_->setAlignment(Qt::AlignCenter);

    secondary_->setVisible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(0);
    layout->addWidget(title_);
    layout->addWidget(primary_, 1);
    layout->addWidget(secondary_);

    // Base style. Use a property for state so you can change at runtime.
    setProperty("state", "normal");
    setStyleSheet(R"(
QFrame {
  border: 2px solid #404040;
  border-radius: 2px;
  background: #101010;
}
QLabel { padding: 0px; margin: 0px; }

QLabel#title     { color: #B0B0B0; }
QLabel#primary   { color: #F0F0F0; }
QLabel#secondary { color: #C8C8C8; }

QFrame[state="warn"] { border-color: #b37a00; }
QFrame[state="err"]  { border-color: #a00000; }
QFrame[state="ok"]   { border-color: #2e7d32; }
)");
  }

  QLabel *primaryLabel() const { return primary_; }
  QLabel *secondaryLabel() const { return secondary_; }

  void setPrimaryPointSize(int pt, bool bold = true) {
    auto f = primary_->font();
    f.setPointSize(pt);
    f.setBold(bold);
    primary_->setFont(f);
  }

  void setSecondaryPointSize(int pt, bool bold = false) {
    auto f = secondary_->font();
    f.setPointSize(pt);
    f.setBold(bold);
    secondary_->setFont(f);
  }

  void setTitlePointSize(int pt, bool bold = false) {
    auto f = title_->font();
    f.setPointSize(pt);
    f.setBold(bold);
    title_->setFont(f);
  }

  void setState(const char *state) {
    setProperty("state", state);
    style()->unpolish(this);
    style()->polish(this);
    update();
  }

  void setSecondaryVisible(bool v) { secondary_->setVisible(v); }

private:
  QLabel *title_;
  QLabel *primary_;
  QLabel *secondary_;
};

class DynamicTileConfig {
public:
  uint16_t primary_point_size;
  uint16_t secondary_point_size;
  bool show_secondary;
};

struct NamedColor {
  const char* name;
  QColor color;
};

static const NamedColor kNamedColors[] = {
  {"Black", QColor("#000000")},
  {"Dark Gray", QColor("#202020")},
  {"Slate", QColor("#2F4F4F")},
  {"Navy", QColor("#001f3f")},
  {"Maroon", QColor("#800000")},
  {"Dark Green", QColor("#004d00")},
  {"White", QColor("#FFFFFF")},
  {"Red", QColor("#FF0000")},
  {"MediumSpringGreen", QColor("#00FA9A")},
  {"LightSeaGreen", QColor("#20B2AA")},
  {"SteelBlue", QColor("#4682B4")},
  {"DeepSkyBlue", QColor("#00BFF")}
};