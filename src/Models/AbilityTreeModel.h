#pragma once

#include <array>

#include "BaseTreeModel.h"

class AbilityTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	std::array<std::string, 3> subCategories = {
		"Units",
		"Heroes",
		"Items"
	};

	BaseTreeItem* getFolderParent(const std::string& id) const override;


public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit AbilityTreeModel(QObject* parent = nullptr);
};