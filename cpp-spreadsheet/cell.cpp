#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

// Реализуйте следующие методы
Cell::Cell() {}

Cell::~Cell() {}

void Cell::Set(SheetInterface& sheet, Position pos, std::string text) {
	referenced_cells_.clear(); // Очищаем формулу ячейки
	if (text.size() > 1 && text.at(0) == FORMULA_SIGN) {
		auto formula = ParseFormula({ text.begin() + 1, text.end() });
		const Value temp_value = value_;
		const std::string temp_text = text_;
		const std::vector<Position> temp_referenced_cells = referenced_cells_;
		if (std::holds_alternative<double>(formula.get()->Evaluate(sheet))) {
			value_ = std::get<double>(formula.get()->Evaluate(sheet));
		}
		else if (std::holds_alternative<FormulaError>(formula.get()->Evaluate(sheet))) {
			value_ = std::get<FormulaError>(formula.get()->Evaluate(sheet));
		}
		text_ = "=" + formula.get()->GetExpression();
		DeleteUnreferencedCells(sheet, pos, formula.get()->GetReferencedCells());
		UpdateCellConnections(sheet, pos, formula.get()->GetReferencedCells(), temp_value, temp_text, temp_referenced_cells);
	}
	else if (text.size() && text.at(0) == ESCAPE_SIGN) {
		text_ = text;
		value_ = std::string{text.begin() + 1, text.end()};
	}
	else if (!text.empty()) {
		text_ = text;
		bool is_text = IsText(text);
		is_text ? value_ = text : value_ = std::stod(text);
	}
	else {
		text_ = "";
		value_ = 0.0;
	}
	CalcCellGraph(sheet, pos);
	InvalidateCache(sheet, pos);
}

void Cell::Clear() {
	text_ = "";
	value_ = 0.0;
	referenced_cells_.clear();
}

Cell::Value Cell::GetValue() const {
	return value_;
}

std::string Cell::GetText() const {
	return text_;
}

std::vector<Position> Cell::GetReferencedCells() const {
	return referenced_cells_;
}

void Cell::DeleteUnreferencedCells(SheetInterface& sheet, Position pos, std::vector<Position> new_referenced_cells) {
	std::map<Position, std::set<Position>>& referenced_cells = sheet.GetReferencedCellsMap();
	std::map<Position, std::vector<std::pair<Position, int>>>& dependent_graph = sheet.GetGraph();
	std::set<Position> must_to_erase;
	for (Position old_referenced_cell_position : referenced_cells[pos]) {
		if (!std::count(new_referenced_cells.begin(),
			new_referenced_cells.end(), old_referenced_cell_position)) {
			must_to_erase.insert(old_referenced_cell_position);
		}
	}
	// Удаляем зависимость от ячеек которые удалили из формулы
	for (Position old_referenced_cell_position : must_to_erase) {
		referenced_cells.at(pos).erase(old_referenced_cell_position);
		dependent_graph[old_referenced_cell_position].erase(std::find_if(dependent_graph[old_referenced_cell_position].begin(),
			dependent_graph[old_referenced_cell_position].end(), [pos](const std::pair<Position, int> d_cells) {
				return d_cells.first == pos;
			}));
		// Если ячейка пустая и на неё больше никто не ссылается, удаляем её
		if (!dependent_graph[old_referenced_cell_position].size() &&
			sheet.GetCell(old_referenced_cell_position)->GetText().empty())
			sheet.ClearCell(old_referenced_cell_position);
	}
}

void Cell::UpdateCellConnections(SheetInterface& sheet, Position pos, std::vector<Position> new_referenced_cells, 
	Value temp_value, std::string temp_text, std::vector<Position> temp_referenced_cells) {
	std::map<Position, std::set<Position>>& referenced_cells = sheet.GetReferencedCellsMap();
	std::map<Position, std::vector<std::pair<Position, int>>>& dependent_graph = sheet.GetGraph();
	for (const auto& cell_position : new_referenced_cells) {
		referenced_cells_.push_back(cell_position);
		referenced_cells[pos].insert(cell_position);
		// Проверка на зацикленность
		try {
			CheckCircle(sheet, pos, sheet.GetReferencedCellsMap()[cell_position]);
		}
		catch (const CircularDependencyException& error) {
			value_ = temp_value;
			text_ = temp_text;
			referenced_cells_ = temp_referenced_cells;
			referenced_cells[pos].erase(cell_position);
			throw error;
		}
		if (sheet.GetCell(cell_position) == nullptr) sheet.SetCell(cell_position, "");
		if (!std::count_if(dependent_graph[cell_position].begin(), dependent_graph[cell_position].end(), [pos](std::pair<Position, int> cell) {
			return cell.first == pos;
			})) dependent_graph[cell_position].push_back({ pos, 0 });
	}
}

void Cell::CalcCellGraph(SheetInterface& sheet, Position pos) {
	std::map<Position, std::vector<std::pair<Position, int>>>& graph = sheet.GetGraph();
	std::vector<std::pair<Position, int>>& cell_graph = graph[pos];
	// Обнуляем веса графа для пересчёта
	for (auto& cell : cell_graph) {
		cell.second = 0;
	}

	for (auto& dependent_cell : cell_graph) { // Перебираем все зависимые ячейки
		CalcMaxCellWeight(sheet, pos, dependent_cell); // Заходим в рекурсию
	}
	// Сортируем граф по дальности зависимой ячейки
	std::sort(cell_graph.begin(), cell_graph.end(), [](const std::pair<Position, int>& lhs, const std::pair<Position, int>& rhs) {
		return lhs.second < rhs.second;
		});
}

void Cell::CalcMaxCellWeight(SheetInterface& sheet, Position pos, std::pair<Position, int> dependent_cell) {
	std::map<Position, std::vector<std::pair<Position, int>>>& graph = sheet.GetGraph();
	std::vector<std::pair<Position, int>>& next_dependent_cells = graph[dependent_cell.first]; // Получаем список зависимых ячеек от следующей ячейки

	std::map<Position, std::set<Position>>& referenced_cells = sheet.GetReferencedCellsMap();

	for (const auto& next_dependent_cell : next_dependent_cells) { // Перебираем все зависимые ячейки от зависящей ячейки
		auto next_referenced_cells = referenced_cells[next_dependent_cell.first]; // Получаем список ячеек участвующих в формуле зависимой ячейки от зависимой ячейки
		if (std::count(next_referenced_cells.begin(), next_referenced_cells.end(), pos)) { // Если в формуле зависимой ячейки от зависимой ячейки участвует измененная ячейка
			auto weighted_cell = std::find_if(graph.at(pos).begin(), graph.at(pos).end(), [&next_dependent_cell](const std::pair<Position, int>& cell_graph) {
				return cell_graph.first == next_dependent_cell.first;
				});
			weighted_cell.operator*().second++;
			CalcMaxCellWeight(sheet, pos, next_dependent_cell); // Заходим в рекурисю
		}
	}
}

void Cell::InvalidateCache(SheetInterface& sheet, Position pos) {
	for (const std::pair<Position, int>& position_weight : sheet.GetGraph().at(pos)) {
		std::string text = sheet.GetCell(position_weight.first)->GetText();
		std::string temp = { text.begin() + 1, text.end() };
		auto formula = ParseFormula(temp);
		if (std::holds_alternative<double>(formula.get()->Evaluate(sheet))) {
			sheet.GetCell(position_weight.first)->SetValue(std::get<double>(formula.get()->Evaluate(sheet)));
		}
	}
}

void Cell::CheckCircle(SheetInterface& sheet, Position pos, std::set<Position> referenced_cells) {
	if (referenced_cells.count(pos)) throw CircularDependencyException("Circle!");
	for (const auto& referenced_cell : referenced_cells) {
		CheckCircle(sheet, pos, sheet.GetReferencedCellsMap()[referenced_cell]);
	}
}

void Cell::SetValue(double value) {
	value_ = value;
}

bool Cell::IsText(const std::string text) const {
	bool result = false;
	for (const char c : text) {
		if (!std::isdigit(c)) {
			result = true;
			break;
		}
	}
	return result;
}
