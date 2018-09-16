#include "stdafx.h"

DoodadPalette::DoodadPalette(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.create();
	map.brush = &brush;

	for (auto&&[key, value] : world_edit_data.section("TileSets")) {
		const std::string tileset_key = value.front();
		ui.tileset->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DoodadCategories")) {
		const std::string tileset_key = value.front();
		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DestructibleCategories")) {
		const std::string text = value.front();
		ui.type->addItem(QString::fromStdString(text), QString::fromStdString(key));
	}

	QRibbonSection* selection_section = new QRibbonSection;

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	connect(new QShortcut(Qt::Key_Space, parent), &QShortcut::activated, selection_mode, &QToolButton::click);

	QRibbonSection* placement_section = new QRibbonSection;
	placement_section->setText("Placement");
	
	QRibbonButton* random_rotation = new QRibbonButton;
	random_rotation->setText("Random\nRotation");
	random_rotation->setIcon(QIcon("Data/Icons/Ribbon/reset32x32.png"));
	random_rotation->setCheckable(true);
	placement_section->addWidget(random_rotation);

	QRibbonButton* random_scale = new QRibbonButton;
	random_scale->setText("Random\nScale");
	random_scale->setIcon(QIcon("Data/Icons/Ribbon/scale32x32.png"));
	random_scale->setCheckable(true);
	placement_section->addWidget(random_scale);


	QRibbonSection* variation_section = new QRibbonSection;
	variation_section->setText("Variations");

	QRibbonButton* random_variation = new QRibbonButton;
	random_variation->setText("Random\nVariation");
	random_variation->setIcon(QIcon("Data/Icons/Ribbon/variation32x32.png"));
	random_variation->setCheckable(true);
	random_variation->setChecked(true);
	variation_section->addWidget(random_variation);
	variation_section->addWidget(variations);

	/*QVBoxLayout* lay = new QVBoxLayout;
	QDoubleSpinBox* but = new QDoubleSpinBox;
	QDoubleSpinBox* butt = new QDoubleSpinBox;

	but-> setStyleSheet(R"(
		QDoubleSpinBox {
			border: 1px solid black;
		}
	)");

	butt->setStyleSheet(R"(
		QDoubleSpinBox {
			border: 1px solid black;
		}
	)");

	QHBoxLayout* tt = new QHBoxLayout;
	tt->addWidget(new QLabel("Min"));
	tt->addWidget(but);

	QHBoxLayout* ttt = new QHBoxLayout;
	ttt->addWidget(new QLabel("Max"));
	ttt->addWidget(butt);

	ttt->setSpacing(6);*/

	//lay->addLayout(tt);
	//lay->addLayout(ttt);
	//lay->addWidget(buttt);

	//section->addLayout(lay);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(placement_section);
	ribbon_tab->addSection(variation_section);

	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });
	connect(random_variation, &QRibbonButton::toggled, [&](bool checked) { brush.random_variation = checked; });

	connect(ui.tileset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.doodads, &QListWidget::itemDoubleClicked, this, &DoodadPalette::selection_changed);

	update_list();
}

DoodadPalette::~DoodadPalette() {
	map.brush = nullptr;
}

bool DoodadPalette::event(QEvent *e) {
	if (e->type() == QEvent::WindowActivate) {
		map.brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Doodad Palette");
	}
	return QWidget::event(e);
}

void DoodadPalette::update_list() {
	ui.doodads->clear();
	
	char selected_tileset = ui.tileset->currentData().toString().toStdString().front();
	std::string selected_category = ui.type->currentData().toString().toStdString();

	bool is_doodad = world_edit_data.key_exists("DoodadCategories", selected_category);
	slk::SLK& slk = is_doodad ? map.doodads.doodads_slk : map.doodads.destructibles_slk;

	for (int i = 1; i < slk.rows; i++) {
		// If the doodad belongs to this tileset
		std::string tilesets = slk.data("tilesets", i);
		if (tilesets != "*" && tilesets.find(selected_tileset) == std::string::npos) {
			continue;
		}

		// If the doodad belongs to this category
		std::string category = slk.data("category", i);
		if (category != selected_category) {
			continue;
		}

		std::string text = slk.data("Name", i);
		if (!is_doodad) {
			text += " " + map.doodads.destructibles_slk.data("EditorSuffix", i);
		}

		QListWidgetItem* item = new QListWidgetItem(ui.doodads);
		item->setText(QString::fromStdString(text));
		item->setData(Qt::UserRole, QString::fromStdString(slk.data(is_doodad ? "doodID" : "DestructableID", i)));
	}
}

void DoodadPalette::selection_changed(QListWidgetItem* item) {
	const std::string id = item->data(Qt::UserRole).toString().toStdString();

	brush.set_doodad(id);
	selection_mode->setChecked(false);

	bool is_doodad = map.doodads.doodads_slk.row_header_exists(id);
	slk::SLK& slk = is_doodad ? map.doodads.doodads_slk : map.doodads.destructibles_slk;

	variations->clear();

	int variation_count = std::stoi(slk.data("numVar", id));
	for (int i = 0; i < variation_count; i++) {
		QRibbonButton* toggle = new QRibbonButton;
		toggle->setCheckable(true);
		toggle->setChecked(true);
		toggle->setText(QString::number(i));
		variations->addWidget(toggle, i % 3, i / 3);
		connect(toggle, &QRibbonButton::toggled, [=](bool checked) {
			if (checked) {
				brush.add_variation(i);
			} else {
				brush.erase_variation(i);
			}
		});
	}
}