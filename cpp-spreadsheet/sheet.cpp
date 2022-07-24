#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        if (GetCell(pos) != nullptr) {
            GetCell(pos)->Set(*this, pos, text);
        }
        else {
            auto cell = CreateCell(pos, text);
            sheet_[pos] = cell.operator*();
            if (pos.row + 1 > size_.rows) size_.rows = pos.row + 1;
            if (pos.col + 1 > size_.cols) size_.cols = pos.col + 1;
        }
    }
    else throw InvalidPositionException("Invalid position set");
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (pos.IsValid()) {
        if (sheet_.count(pos)) return &sheet_.at(pos);
        else return nullptr;
    }
    else throw FormulaException("Invalid position GetCell const");
}
CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (sheet_.count(pos)) return &sheet_.at(pos);
        else return nullptr;
    }
    else throw InvalidPositionException("Invalid position GetCell");
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        if (sheet_.count(pos)) {
            sheet_.at(pos).SetValue(0);
            sheet_.at(pos).CalcCellGraph(*this, pos);
            sheet_.at(pos).InvalidateCache(*this, pos);
            // Удаляем из карты зависимостей формул все ячейки
            for (const auto& cell : sheet_.at(pos).GetReferencedCells()) {
                graph_[cell].erase(std::find_if(graph_[cell].begin(), graph_[cell].end(), [pos](std::pair<Position, int> cell) {
                    return cell.first == pos;
                    }));
            }
            graph_.erase(pos);
            referenced_cells_map_.erase(pos);
            sheet_.at(pos).Clear();
            sheet_.erase(pos);
            if (pos.row + 1 == size_.rows) UpdateRows();
            if (pos.col + 1 == size_.cols) UpdateCols();
        }
    }
    else throw InvalidPositionException("Invalid position clear");
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}

void Sheet::PrintValues(std::ostream& output) const {
    if (size_.rows != 0 && size_.cols != 0) {
        for (int row = 0; row < size_.rows; ++row) {
            for (int col = 0; col < size_.cols; ++col) {
                if (col != 0 ) output << '\t';
                if (sheet_.count({ row, col })) output << sheet_.at({ row, col }).GetValue();
            }
            output << '\n';
        }
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    if (size_.rows != 0 && size_.cols != 0) {
        for (int row = 0; row < size_.rows; ++row) {
            for (int col = 0; col < size_.cols; ++col) {
                if (col != 0) output << '\t';
                if (sheet_.count({ row, col })) output << sheet_.at({ row, col }).GetText();
            }
            output << '\n';
        }
    }
}

std::unique_ptr<Cell> Sheet::CreateCell(Position pos, const std::string& str) {
    std::unique_ptr<Cell> cell = std::make_unique<Cell>();
    cell->Set(*this, pos, str);
    return cell;
}

void Sheet::UpdateRows() {
    int max_row = 0;
    for (const auto& cell : sheet_) {
        if (cell.first.row + 1 > max_row) max_row = cell.first.row + 1;
    }
    size_.rows = max_row;
}

void Sheet::UpdateCols() {
    if (sheet_.size()) {
        size_.cols = std::prev(sheet_.end())->first.col + 1;
    }
    else size_.cols = 0;
}

std::map<Position, std::set<Position>>& Sheet::GetReferencedCellsMap() {
    return referenced_cells_map_;
}

std::map<Position, std::vector<std::pair<Position, int>>>& Sheet::GetGraph() {
    return graph_;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}