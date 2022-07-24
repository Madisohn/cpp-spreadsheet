#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <string>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
            SetReferencedCells(ast_.GetReferencedCells());
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                ast_.Execute(sheet);
            }
            catch (FormulaError &e) {
                return e;
            }
            return ast_.Execute(sheet);
        }
        std::string GetExpression() const override {
            std::ostringstream output;
            ast_.PrintFormula(output);
            return output.str();
        }
        // Соритрует, очищает от повторов и сохраняет ячейки участвующие в формуле
        void SetReferencedCells(const std::forward_list<Position> cells) {
            std::set<Position> temp_referenced_sorted_cells(cells.begin(), cells.end());
            for (Position pos : temp_referenced_sorted_cells) {
                referenced_cells_.push_back(pos);
            }
        }

        virtual std::vector<Position> GetReferencedCells() const override {
            return referenced_cells_;
        }

    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}