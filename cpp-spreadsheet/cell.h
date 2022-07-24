#pragma once

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:


    Cell();
    ~Cell();

    void Set(SheetInterface& sheet, Position pos, std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const;

    // Расчитать расстояние от ячейки до зависимых от неё ячеек
    void CalcCellGraph(SheetInterface& sheet, Position pos);

    // Рекурсивный метод
    // Рассчитывает максимальное расстояние до зависимых ячеек, если в формуле зависимой ячейки от зависимой
    // ячейки участвует эта ячейка этой ячейке добавляется вес(+1), и она становится зависимой ячейкой для передачи в рекурсию
    void CalcMaxCellWeight(SheetInterface& sheet, Position pos, std::pair<Position, int> dependent_cell);

    // Обновляет значение ячеек зависящих от измененной ячейки
    void InvalidateCache(SheetInterface& sheet, Position pos);

    // Ищет зацикленность
    void CheckCircle(SheetInterface& sheet, Position pos, std::set<Position> referenced_cells);

    // Задать значение
    void SetValue(double value);

    // Удаляем из формулы ячейки
    void DeleteUnreferencedCells(SheetInterface& sheet, Position pos, std::vector<Position> new_referenced_cells);

    // Обновляем взаимосвязи между ячейками
    void UpdateCellConnections(SheetInterface& sheet, Position pos, std::vector<Position> new_referenced_cells,
        Value temp_value, std::string temp_text, std::vector<Position> temp_referenced_cells);

    bool IsText(const std::string text) const;
private:
    Value value_;
    std::string text_;
    std::vector<Position> referenced_cells_;
};