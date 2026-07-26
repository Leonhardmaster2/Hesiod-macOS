/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QColorDialog>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/gui/widgets/app_settings_window.hpp"
#include "hesiod/gui/widgets/gui_utils.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/utils.hpp"

namespace hesiod
{

AppSettingsWindow::AppSettingsWindow(QWidget *parent) : QWidget(parent)
{
  Logger::log()->trace("AppSettingsWindow::AppSettingsWindow");

  this->setWindowTitle("Hesiod - Application settings");
  this->setup_layout();
}

void AppSettingsWindow::add_description(const std::string &description, int max_length)
{
  if (description.empty())
    return;

  QLabel *label = new QLabel(wrap_text(description, max_length).c_str(), this);
  label->setWordWrap(true);

  std::string style = std::format(
      "color: {};",
      HSD_CTX.app_settings.colors.text_secondary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, -1);

  this->layout->addWidget(label);
}

void AppSettingsWindow::add_title(const std::string &text, int font_size_delta)
{
  if (text.empty())
    return;

  QLabel *label = new QLabel(text.c_str(), this);

  std::string style = std::format(
      "font-weight: bold; color: {};",
      HSD_CTX.app_settings.colors.text_primary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, font_size_delta);

  this->layout->addWidget(label);
}

void AppSettingsWindow::bind_int(const std::string &label,
                                 int               &value,
                                 int                minimum,
                                 int                maximum,
                                 const std::string &tool_tip)
{
  auto *row = new QWidget(this);
  auto *row_layout = new QHBoxLayout(row);
  row_layout->setContentsMargins(0, 0, 0, 0);

  auto *row_label = new QLabel(label.c_str(), row);
  auto *spin_box = new QSpinBox(row);

  // initialize with current value
  spin_box->setValue(value);

  spin_box->setRange(minimum, maximum);
  spin_box->setFixedWidth(92);
  spin_box->setToolTip(tool_tip.c_str());

  this->connect(spin_box,
                QOverload<int>::of(&QSpinBox::valueChanged),
                this,
                [&value](int v) { value = v; });

  row_layout->addWidget(row_label);
  row_layout->addStretch();
  row_layout->addWidget(spin_box);
  this->layout->addWidget(row);
}

void AppSettingsWindow::bind_bool(const std::string &label,
                                  bool              &state,
                                  const std::string &tool_tip)
{
  auto *check_box = new QCheckBox(label.c_str(), this);
  check_box->setChecked(state);
  check_box->setMinimumHeight(28);
  check_box->setToolTip(tool_tip.c_str());

  this->connect(check_box,
                &QCheckBox::toggled,
                this,
                [&state](bool value) { state = value; });

  this->layout->addWidget(check_box);
}

void AppSettingsWindow::bind_qcolor(const std::string &label,
                                    QColor            &color,
                                    const std::string &tool_tip)
{
  auto *button = new QPushButton(this);
  button->setFixedSize(76, 26);
  button->setToolTip(tool_tip.c_str());

  auto update_button = [button, &color]()
  {
    button->setText(color.name().toUpper());
    button->setStyleSheet(
        QString("background-color: %1; border: 1px solid #444;").arg(color.name()));
  };
  update_button();

  this->connect(
      button,
      &QPushButton::clicked,
      this,
      [this, button, &color, update_button]()
      {
        QColor new_color = QColorDialog::getColor(color,
                                                  this,
                                                  "Select color",
                                                  QColorDialog::ShowAlphaChannel);

        if (!new_color.isValid())
          return;

        color = new_color;
        update_button();

        if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
          apply_global_style(*app);
      });

  auto *row = new QWidget(this);
  auto *row_layout = new QHBoxLayout(row);
  row_layout->setContentsMargins(0, 0, 0, 0);
  row_layout->addWidget(new QLabel(label.c_str(), row));
  row_layout->addStretch();
  row_layout->addWidget(button);
  this->layout->addWidget(row);
}

void AppSettingsWindow::setup_layout()
{
  Logger::log()->trace("AppSettingsWindow::setup_layout");

  AppContext &ctx = HSD_CTX;

  this->layout = new QVBoxLayout(this);
  this->layout->setContentsMargins(24, 20, 24, 8);
  this->layout->setSpacing(8);
  this->setMinimumWidth(560);
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

  {
    auto *header = new QWidget(this);
    auto *header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(0, 0, 0, 8);
    header_layout->setSpacing(14);

    auto *icon = new QLabel(header);
    icon->setPixmap(QIcon(ctx.app_settings.global.icon_path.c_str()).pixmap(48, 48));
    icon->setFixedSize(48, 48);

    auto *header_text = new QWidget(header);
    auto *header_text_layout = new QVBoxLayout(header_text);
    header_text_layout->setContentsMargins(0, 0, 0, 0);
    header_text_layout->setSpacing(2);

    auto *title = new QLabel("Application Settings", header_text);
    resize_font(title, +2);
    title->setStyleSheet("font-weight: bold;");

    auto *description = new QLabel(
        "Control performance and interface preferences. Changes are saved immediately.",
        header_text);
    description->setStyleSheet(
        std::format("color: {};", ctx.app_settings.colors.text_secondary.name().toStdString())
            .c_str());
    description->setWordWrap(true);

    header_text_layout->addWidget(title);
    header_text_layout->addWidget(description);
    header_layout->addWidget(icon);
    header_layout->addWidget(header_text, 1);
    this->layout->addWidget(header);
  }

  // --- Global

  this->add_title("Global", +1);

  this->bind_bool("Create a backup file whenever saving",
                  ctx.app_settings.global.save_backup_file);
  this->bind_int("Number of threads used for OpenMP",
                 ctx.app_settings.global.omp_num_threads,
                 1,
                 64,
                 "How many CPU cores Hesiod may use. The arrows increase or decrease "
                 "the value; leave headroom for the rest of macOS.");

  // --- Interface

  this->add_title("Interface", +1);

  this->bind_bool("Enable data preview in node body",
                  ctx.app_settings.interface.enable_data_preview_in_node_body);
  this->bind_bool("Enable node settings in node body",
                  ctx.app_settings.interface.enable_node_settings_in_node_body);
  this->bind_bool("Enable tool tips", ctx.app_settings.interface.enable_tool_tips);
  this->bind_bool("Enable texture downloader",
                  ctx.app_settings.interface.enable_texture_downloader);
  this->bind_bool("Enable heightmap importer",
                  ctx.app_settings.interface.enable_heightmapper_widget);
  this->bind_bool("Enable example selector at startup",
                  ctx.app_settings.interface.enable_example_selector_at_startup);

  this->add_title("Node Editor", +1);

  this->bind_bool("Show node toolbar in settings panel",
                  ctx.app_settings.node_editor.show_node_toolbar_in_settings_pan);
  this->bind_bool("Show node library panel",
                  ctx.app_settings.node_editor.show_node_library_pan);
  this->bind_bool("Show node settings panel",
                  ctx.app_settings.node_editor.show_node_settings_pan);
  this->bind_bool("Show viewer in the main window",
                  ctx.app_settings.node_editor.show_viewer);
  this->bind_bool("Disable graph interaction while computing",
                  ctx.app_settings.node_editor.disable_during_update);
  this->bind_bool("Allow node groups", ctx.app_settings.node_editor.enable_node_groups);

  this->add_title("Appearance", +1);
  this->add_description("These colors update the macOS theme immediately.");
  this->bind_qcolor("Accent and selection", ctx.app_settings.colors.accent);
  this->bind_qcolor("Hover surface", ctx.app_settings.colors.hover);
  this->bind_qcolor("Pressed surface", ctx.app_settings.colors.pressed);

  this->add_title("Viewport", +1);

  this->bind_bool("Add border skirt to the heightmap",
                  ctx.app_settings.viewer.add_heighmap_skirt);
  this->bind_int("Default preview width", ctx.app_settings.node_editor.preview_w, 32, 512);
  this->bind_int("Default preview height", ctx.app_settings.node_editor.preview_h, 32, 512);

  // --- Reset

  this->layout->addSpacing(8);

  auto *reset_button = new QPushButton("Reset Settings");
  reset_button->setMinimumHeight(32);

  reset_button->setToolTip("Restore every preference to its default value.");

  this->connect(reset_button,
                &QPushButton::clicked,
                this,
                [this, &ctx]()
                {
                  QMessageBox::StandardButton reply = QMessageBox::warning(
                      this,
                      "Reset Settings",
                      "This will reset all application settings to their "
                      "default values.\n\n"
                      "This action cannot be undone.",
                      QMessageBox::Yes | QMessageBox::Cancel,
                      QMessageBox::Cancel);

                  if (reply != QMessageBox::Yes)
                    return;

                  ctx.reset_settings();

                  QMessageBox::information(
                      this,
                      "Settings Reset",
                      "Settings have been reset.\n"
                      "Please restart the application for all changes to take effect.");
                });

  this->layout->addWidget(reset_button);
}

} // namespace hesiod
