#include "ObjectEditor.h"

#include <QSortFilterProxyModel>

#include "SingleModel.h"
#include <QTableView>

void ObjectEditor::item_clicked(const QModelIndex& index, Category category) {
	BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());
	if (item->tableRow > 0) {
		ads::CDockWidget* dock_tab = new ads::CDockWidget("");
		dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);

		connect(dock_tab, &ads::CDockWidget::closeRequested, [&, dock_tab]() {
			if (dock_area->dockWidgets().contains(dock_tab) && dock_area->dockWidgetsCount() == 1) {
				dock_area = nullptr;
			}
		});

		QTableView* view = new QTableView;
		TableDelegate* delegate = new TableDelegate;
		view->setItemDelegate(delegate);
		view->horizontalHeader()->hide();
		view->setAlternatingRowColors(true);
		view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
		view->setIconSize({ 24, 24 });
		dock_tab->setWidget(view);

		SingleModel* single_model;
		switch (category) {
			case Category::unit: {
				std::string id = units_slk.data("unitid", item->tableRow);

				single_model = new SingleModel(&units_slk, &units_meta_slk, this);
				single_model->setSourceModel(unitTableModel);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(units_slk.data("name", item->tableRow)));
				dock_tab->setIcon(unitTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::doodad: {
				std::string id = doodads_slk.data("doodid", item->tableRow);

				single_model = new SingleModel(&doodads_slk, &doodads_meta_slk, this);
				single_model->setSourceModel(doodadTableModel);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(doodads_slk.data("name", item->tableRow)));
				dock_tab->setIcon(doodadTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::ability: {
				std::string id = abilities_slk.data("alias", item->tableRow);

				single_model = new SingleModel(&abilities_slk, &abilities_meta_slk, this);
				single_model->setSourceModel(abilityTableModel);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(abilities_slk.data("name", item->tableRow)));
				dock_tab->setIcon(abilityTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			default:
				return;
		}
		view->setModel(single_model);

		if (dock_area == nullptr) {
			dock_area = dock_manager->addDockWidget(ads::RightDockWidgetArea, dock_tab, dock_area);
		} else {
			dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
		}
	}
}

ObjectEditor::ObjectEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::AllTabsHaveCloseButton);
	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::DockAreaDynamicTabsMenuButtonVisibility);
	dock_manager->setStyleSheet("");
	setCentralWidget(dock_manager);

	unitTableModel = new TableModel(&units_slk, &units_meta_slk, this);
	unitTreeModel = new UnitTreeModel(this);
	unitTreeModel->setSourceModel(unitTableModel);
	unit_explorer->setModel(unitTreeModel);
	unit_explorer->header()->hide();
	
	ads::CDockWidget* unit_tab = new ads::CDockWidget("Units");
	unit_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	unit_tab->setWidget(unit_explorer);
	auto t = dock_manager->addDockWidget(ads::LeftDockWidgetArea, unit_tab);

	itemTableModel = new TableModel(&items_slk, &items_meta_slk, this);
	itemTreeModel = new ItemTreeModel(this);
	itemTreeModel->setSourceModel(itemTableModel);
	item_explorer->setModel(itemTreeModel);
	item_explorer->header()->hide();

	ads::CDockWidget* item_tab = new ads::CDockWidget("Items");
	item_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	item_tab->setWidget(item_explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, item_tab, t);

	doodadTableModel = new TableModel(&doodads_slk, &doodads_meta_slk, this);
	doodadTreeModel = new DoodadTreeModel(this);
	doodadTreeModel->setSourceModel(doodadTableModel);
	doodad_explorer->setModel(doodadTreeModel);
	doodad_explorer->header()->hide();

	ads::CDockWidget* doodad_tab = new ads::CDockWidget("Doodads");
	doodad_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	doodad_tab->setWidget(doodad_explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, doodad_tab, t);

	ads::CDockWidget* destructible_tab = new ads::CDockWidget("Destructibles");
	destructible_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	//destructible_tab->setWidget(explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, destructible_tab, t);

	abilityTableModel = new TableModel(&abilities_slk, &abilities_meta_slk, this);
	abilityTreeModel = new AbilityTreeModel(this);
	abilityTreeModel->setSourceModel(abilityTableModel);
	ability_explorer->setModel(abilityTreeModel);
	ability_explorer->header()->hide();

	ads::CDockWidget* ability_tab = new ads::CDockWidget("Abilities");
	ability_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	ability_tab->setWidget(ability_explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, ability_tab, t);

	ads::CDockWidget* upgrade_tab = new ads::CDockWidget("Upgrades");
	upgrade_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	//upgrade_tab->setWidget(explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, upgrade_tab, t);

	ads::CDockWidget* buff_tab = new ads::CDockWidget("Buffs");
	buff_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	//buff_tab->setWidget(explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, buff_tab, t);

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::unit); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::doodad); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::ability); });

	show();
}