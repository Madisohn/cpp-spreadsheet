#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Создаёт абстрактную ячейку
    std::unique_ptr<Cell> CreateCell(Position pos, const std::string& str);

    // Обновляет высоту таблицы
    void UpdateRows();
    // Обновляет ширину таблицы
    void UpdateCols();

    std::map<Position, std::set<Position>>& GetReferencedCellsMap();
    std::map<Position, std::vector<std::pair<Position, int>>>& GetGraph();

private:
    std::map<Position, Cell> sheet_;
    std::map<Position, std::set<Position>> referenced_cells_map_;
    std::map<Position, std::vector<std::pair<Position, int>>> graph_;
    Size size_ = {0, 0};
};