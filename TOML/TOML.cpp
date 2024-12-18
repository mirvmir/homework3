#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <locale>
#include <map> 
#include <stack>
#include <cctype>
#include <sstream>
#include <algorithm>

using namespace std;

// ЛЕКСИЧЕСКИЙ АНАЛИЗАТОР
// Состояния автомата
enum states {
    H,     // Начальное состояние
    ID,    // Идентификатор или ключевое слово
    NUM,   // Число
    STR,   // Строковый литерал
    REF,   // Ссылка на константу
    C1,    // Начало комментария
    C2,    // Многострочный комментарий
    C3,    // Однострочный комментарий
    DLM,   // Ограничитель
    ERR    // Ошибка
};

// Типы токенов
enum TokenType {
    KWORD = 1,      // Ключевые слова
    IDENT = 2,      // Идентификаторы
    NUMERIC = 3,    // Числа
    DELIM = 4,      // Ограничители
    COMMENTS = 5,   // Комментарий
    STRING = 6,     // Строковый литерал
    REFERENCE = 7   // Ссылка на константу
};

// Структура токена
struct Token {
    TokenType type;
    string value;
    int index;  // Индекс токена в таблице
};
vector<Token> tokens;

// Ключевые слова
vector<string> TW = { "set", "true", "false" };
// Ограничители
vector<string> TL = { "%{", "%}", "{", ":", ";", "}", "--", "$", "[", "]", "=", "\"" };
// Таблицы для комментариев, идентификаторов и чисел
vector<string> COM;
vector<string> TI;
vector<string> TN;

// Функции для поиска индексов
int findKeyword(string word) {
    for (int i = 0; i < TW.size(); ++i) {
        if (TW[i] == word) return i;
    }
    return -1;
}

int findDelimiter(string delim) {
    for (int i = 0; i < TL.size(); ++i) {
        if (TL[i] == delim) return i;
    }
    return -1;
}

// Проверка, является ли строка разделителем
bool isdelimiter(string delim) {
    return findDelimiter(delim) != -1;
}

// Добавление токена в список токенов
void addToken(TokenType type, string value, int index = 0) {
    tokens.push_back({ type, value, index });
}

// Лексический анализатор с конечным автоматом
bool scanner(string input, vector<Token>& tokens) {
    enum states CS = H; // Текущее состояние
    int length = input.size();
    int i = 0;
    string current_token;

    while (true) {
        if (i >= length && CS == H) break; // Завершаем, если достигли конца и находимся в начальном состоянии

        char c = '\0';
        if (i < length) c = input[i];

        switch (CS) {
        case H: { // Начальное состояние
            while (i < length && isspace(c)) {
                i++;
                if (i < length) c = input[i];
            }
            if (i >= length) break;
            if (isalpha(c)) {
                current_token.clear();
                current_token += c;
                CS = ID;
                i++;
            }
            else if (isdigit(c)) {
                current_token.clear();
                current_token += c;
                CS = NUM;
                i++;
            }
            else if (c == '%') {
                current_token.clear();
                CS = C1;
                current_token += c;
                i++;
            }
            else if (c == '-') {
                current_token.clear();
                CS = C1;
                current_token += c;
                i++;
            }
            else if (c == '"') {
                current_token.clear();
                CS = STR;
                i++;
            }
            else if (c == '$') {
                current_token.clear();
                current_token += c;
                CS = REF;
                i++;
            }
            else if (isdelimiter(string(1, c))) {
                current_token.clear();
                current_token += c;
                CS = DLM;
                i++;
            }
            else if (c == '\0') {
                break; // Конец ввода
            }
            else {
                CS = ERR;
                i++;
            }
            break;
        }

        case ID: { // Идентификатор или ключевое слово
            while (i < length && (isalnum(input[i]) || input[i] == '_')) {
                current_token += input[i++];
            }
            int keywordIndex = findKeyword(current_token);
            if (keywordIndex != -1) {
                addToken(KWORD, current_token, keywordIndex);
            }
            else {
                int identIndex = TI.size();
                TI.push_back(current_token);
                addToken(IDENT, current_token, identIndex);
            }
            CS = H;
            break;
        }

        case NUM: { // Число
            while (i < length && isdigit(input[i])) {
                current_token += input[i++];
            }
            int numIndex = TN.size();
            TN.push_back(current_token);
            addToken(NUMERIC, current_token, numIndex);
            CS = H;
            break;
        }

        case STR: { // Строковый литерал
            while (i < length && input[i] != '"') {
                current_token += input[i++];
            }
            if (i < length && input[i] == '"') {
                i++; // Пропускаем закрывающую кавычку
                addToken(STRING, current_token);
                CS = H;
            }
            else {
                CS = ERR; // Незакрытая строка
            }
            break;
        }

        case REF: { // Ссылка на константу $[имя]
            if (i < length && input[i] == '[') {
                i++; // Пропускаем '['
                current_token += '[';
                string ref_name;
                while (i < length && isalnum(input[i])) {
                    ref_name += input[i++];
                }
                if (i < length && input[i] == ']') {
                    i++; // Пропускаем ']'
                    current_token += ref_name + ']';
                    addToken(REFERENCE, ref_name);
                    CS = H;
                }
                else {
                    CS = ERR; // Ошибка в ссылке
                }
            }
            else {
                CS = ERR; // Ошибка в ссылке
            }
            break;
        }

        case C1: { // Начало комментария
            if (current_token == "%" && i < length && input[i] == '{') {
                current_token.clear();
                addToken(DELIM, "%{", findDelimiter("%{"));
                CS = C2; // Многострочный комментарий
            }
            else if (current_token == "-" && i < length && input[i] == '-') {
                current_token += input[i++];
                addToken(DELIM, "--", findDelimiter("--"));
                CS = C3; // Однострочный комментарий            
            }
            else {
                CS = ERR;
            }
            break;
        }

        case C2: { // Многострочный комментарий
            while (i + 1 < length) {
                if (input[i] == '%' && input[i + 1] == '}') {
                    i += 2; // Пропускаем закрывающий символ комментария
                    COM.push_back(current_token);
                    int comIndex = COM.size() - 1;
                    addToken(COMMENTS, current_token, comIndex);
                    addToken(DELIM, "%}", findDelimiter("%}"));
                    CS = H; // Переходим в начальное состояние
                    break;
                }
                else {
                    current_token += input[i];
                    i++;
                }
            }
            if (i >= length) {
                CS = ERR; // Ошибка, если достигли конца ввода без закрывающего символа
            }
            break;
        }

        case C3: { // Однострочный комментарий
            while (i < length) {
                if (input[i] == '\n') { // Проверка на конец строки
                    COM.push_back(current_token);
                    int comIndex = COM.size() - 1;
                    addToken(COMMENTS, current_token, comIndex);
                    i++;
                    CS = H; // Переходим в начальное состояние
                    break;
                }
                else {
                    current_token += input[i];
                    i++;
                }
            }
            if (i >= length) { // Если достигли конца ввода, добавляем комментарий
                COM.push_back(current_token);
                int comIndex = COM.size() - 1;
                addToken(COMMENTS, current_token, comIndex);
                CS = H;
            }
            break;
        }

        case DLM: { // Ограничители
            int index = findDelimiter(current_token);
            addToken(DELIM, current_token, index);
            CS = H;
            break;
        }

        case ERR: {
            cout << "Лексическая ошибка: неожиданный символ на позиции " << i << endl;
            CS = H;
            exit(1);
            break;
        }
        }
        if (i >= length && CS == H) break;
    }
    addToken(IDENT, "end");

    return true;
}

// Вывод токенов
void printTokens(vector<Token>& tokens) {
    for (auto& token : tokens) {
        switch (token.type) {
        case KWORD:
            cout << "(1," << token.index << ") Keyword: " << token.value << endl;
            break;
        case IDENT:
            cout << "(2," << token.index << ") Identifier: " << token.value << endl;
            break;
        case NUMERIC:
            cout << "(3," << token.index << ") Number: " << token.value << endl;
            break;
        case DELIM:
            cout << "(4," << token.index << ") Delimiter: " << token.value << endl;
            break;
        case COMMENTS:
            cout << "(5," << token.index << ") Comments: " << token.value << endl;
            break;
        case STRING:
            cout << "(6) String: " << token.value << endl;
            break;
        case REFERENCE:
            cout << "(7) Reference: " << token.value << endl;
            break;
        }
    }
}

// СИНТАКСИЧЕСКИЙ АНАЛИЗАТОР
// Структура узла AST
class ASTNode {
public:
    string type;
    string value;
    ASTNode* left;   // Левый потомок
    ASTNode* right;  // Правый потомок
    bool processed;  // Флаг обработки узла

    ASTNode(string type, string value = "")
        : type(type), value(value), left(nullptr), right(nullptr), processed(false) {}

    void print(int depth = 0) {
        for (int i = 0; i < depth; i++) {
            cout << "  ";
        }
        cout << type << ":" << value << std::endl;
        if (left) left->print(depth + 1);
        if (right) right->print(depth + 1);
    }
};
ASTNode* root = new ASTNode("S");

// Отображение типов токенов
map<TokenType, string> tokenTypeNames;
void initializeTokenTypeNames() {
    tokenTypeNames[KWORD] = "KWORD";
    tokenTypeNames[IDENT] = "IDENT";
    tokenTypeNames[NUMERIC] = "NUMERIC";
    tokenTypeNames[COMMENTS] = "COMMENTS";
    tokenTypeNames[DELIM] = "DELIM";
    tokenTypeNames[STRING] = "STRING";
    tokenTypeNames[REFERENCE] = "REFERENCE";
}

int lineIndex = 1;
int currentIndex = 0;
int currentTokenIndex = 0;

// Функция для получения текущего токена
Token currentToken() {
    return tokens[currentTokenIndex];
}

// Переход к следующему токену
void nextToken() {
    if (currentTokenIndex < tokens.size() - 1) {
        currentTokenIndex++;
        currentIndex++;
    }
    else {
        cout << "Ошибка: Программа завершилась раньше, чем ожидалось." << endl;
        exit(1);
    }
}

// Функция для обработки ошибок
void error(string message) {
    cout << "Синтаксическая ошибка: " << message << " в строке " << lineIndex << " по индексу " << currentIndex << endl;
    system("pause");
    exit(1);
}

// Функция match для проверки типа токена и перехода к следующему
void match(TokenType expectedType, string expectedValue = "") {
    Token token = currentToken();

    // Проверка типа токена
    if (token.type != expectedType) {
        error("Поступил тип данных " + tokenTypeNames[token.type] + ", а ожидался " + tokenTypeNames[expectedType]);
    }

    // Проверка значения токена, если оно передано
    if (!expectedValue.empty() && token.value != expectedValue) {
        error("Поступил тип данных " + tokenTypeNames[token.type] + ", а ожидался " + tokenTypeNames[expectedType]);
    }

    // Переход к следующему токену, если проверка пройдена
    nextToken();
}

ASTNode* S();
ASTNode* Dictionary();
ASTNode* Comment();
ASTNode* Assignment();
ASTNode* Translation();
ASTNode* Reference();
ASTNode* Value();

// Функция для разбора правила S
ASTNode* S() {
    ASTNode* newNode = root;
    ASTNode* rightNode;

    do {
        currentIndex = 0;
        if (currentToken().value == "%{" || currentToken().value == "--") {
            {
                if (!newNode->left)
                    newNode->left = Comment();
                else if (!newNode->right)
                    newNode->right = Comment();
                else
                {
                    rightNode = newNode->right;
                    newNode->right = new ASTNode("");
                    newNode = newNode->right;
                    newNode->left = rightNode;
                    newNode->right = Comment();
                }
            }
            lineIndex += 1;
        }
        else if (currentToken().value == "{") {
            if (!newNode->left)
                newNode->left = Dictionary();
            else if (!newNode->right)
                newNode->right = Dictionary();
            else
            {
                rightNode = newNode->right;
                newNode->right = new ASTNode("");
                newNode = newNode->right;
                newNode->left = rightNode;
                newNode->right = Dictionary();
            }
            lineIndex += 1;
        }
        else {
            if (currentToken().value == "set") {
                {
                    if (!newNode->left)
                        newNode->left = Translation();
                    else if (!newNode->right)
                        newNode->right = Translation();
                    else
                    {
                        rightNode = newNode->right;
                        newNode->right = new ASTNode("");
                        newNode = newNode->right;
                        newNode->left = rightNode;
                        newNode->right = Translation();
                    }
                }
            }
            else {
                {
                    if (!newNode->left)
                        newNode->left = Assignment();
                    else if (!newNode->right)
                        newNode->right = Assignment();
                    else
                    {
                        rightNode = newNode->right;
                        newNode->right = new ASTNode("");
                        newNode = newNode->right;
                        newNode->left = rightNode;
                        newNode->right = Assignment();
                    }
                }
            }
            if (currentToken().value == ";") {
                lineIndex += 1;
                nextToken();
            }
            else {
                error("Ожидалось ';' после " + currentToken().value);
            }
        }
    } while (currentToken().value != "end");

    return newNode;
}

// Функция для разбора комментария
ASTNode* Comment() {
    ASTNode* node = new ASTNode("Comment");

    // Проверка многострочного комментария
    if (currentToken().value == "%{") {
        match(DELIM, "%{");
        node->left = new ASTNode("multiline", currentToken().value);
        match(COMMENTS);
        match(DELIM, "%}");
    }
    // Проверка однострочного комментария
    else if (currentToken().value == "--") {
        match(DELIM, "--");
        node->left = new ASTNode("single-line", currentToken().value);
        match(COMMENTS);
    }
    else {
        error("Ожидался комментарий");
    }

    return node;
}

// Функция для разбора словарей
ASTNode* Dictionary() {
    ASTNode* node = new ASTNode("Dictionary");

    match(DELIM, "{");

    while (currentToken().value != "}") {
        ASTNode* newNode = new ASTNode("Key", currentToken().value);
        match(IDENT);

        match(DELIM, ":");

        newNode->right = Value();

        if (!node->left) {
            node->left = newNode;
        }
        else {
            ASTNode* temp = node->left;
            while (temp->right) temp = temp->right;
            temp->right = newNode;
        }

        match(DELIM, ";");
    }

    match(DELIM, "}");
    return node;
}

// Функция для разбора значений
ASTNode* Value() {
    if (currentToken().type == STRING) {
        ASTNode* node = new ASTNode("String", currentToken().value);
        match(STRING);
        return node;
    }
    else if (currentToken().value == "{") {
        return Dictionary(); // Рекурсивный вызов для вложенных словарей
    }
    else if (currentToken().value == "false" || currentToken().value == "true") {
        ASTNode* node = new ASTNode("Boolean", currentToken().value);
        match(IDENT);
        return node;
    }
    else if (currentToken().type == NUMERIC) {
        ASTNode* node = new ASTNode("Number", currentToken().value);
        match(NUMERIC);
        return node;
    }
    else if (currentToken().type == REFERENCE) {
        return Reference();
    }
    else {
        error("Ожидалось значение");
        return nullptr;
    }
}

// Функция для разбора ссылок на константы
ASTNode* Reference() {
    ASTNode* node = new ASTNode("Reference", currentToken().value);
    match(REFERENCE);
    return node;
}

// Функция для разбора константных значений
ASTNode* Translation() {
    ASTNode* node = new ASTNode("Translation");

    match(KWORD, "set");
    node->left = new ASTNode("Identifier", currentToken().value);
    match(IDENT);
    match(DELIM, "=");

    node->right = Value();

    return node;
}

// Функция для разбора присваивания
ASTNode* Assignment() {
    ASTNode* node = new ASTNode("Assignment");

    node->left = new ASTNode("Identifier", currentToken().value);
    match(IDENT);
    match(DELIM, "=");
    node->right = Reference();

    return node;
}

// СЕМАНТИЧЕСКИЙ АНАЛИЗАТОР
// Переменная для хранения сгенерированного TOML-кода
string tomlCode;

// Глобальная таблица символов для хранения всех ключей
map<string, string> globalSymbols;

// Функция для обработки ошибок
void semanticError(string message) {
    cout << "Семантическая ошибка: " << message << endl;
    system("pause");
    exit(1);
}

// Объявление переменной или константы в глобальной области видимости
void declareVariable(string varName, string varType) {
    auto it = globalSymbols.find(varName);
    if (it != globalSymbols.end()) {
        if (it->second == "const") {
            semanticError("Константа '" + varName + "' уже объявлена и не может быть изменена.");
        }
        if (varType == "const") {
            semanticError("Переменная '" + varName + "' уже объявлена и не может быть переопределена как константа.");
        }
        // Если переменная уже объявлена как 'var', разрешаем переопределение без ошибки
    }
    globalSymbols[varName] = varType;
}

// Поиск переменной или константы в глобальной области видимости
string lookupVariable(string varName) {
    auto it = globalSymbols.find(varName);
    if (it != globalSymbols.end()) {
        return it->second;
    }
    return "";
}

// Получение значения константы
string getConstantValue(string constName) {
    auto it = globalSymbols.find(constName);
    if (it != globalSymbols.end()) {
        return it->second;
    }
    else {
        semanticError("Константа '" + constName + "' не определена");
        return "";
    }
}

// Рекурсивная функция для семантического анализа AST
void semanticAnalysis(ASTNode* node, string currentPath = "") {
    if (!node) return;

    // Проверяем, был ли узел уже обработан
    if (node->processed) {
        return;
    }

    // Устанавливаем флаг обработки
    node->processed = true;

    if (node->type == "S") {
        // Корневой узел
        semanticAnalysis(node->left, currentPath);
        semanticAnalysis(node->right, currentPath);
    }
    else if (node->type == "Translation") {
        // Обработка объявления константы с использованием 'set'
        string constName = node->left->value;

        // Объявляем или обновляем константу
        declareVariable(constName, "const");

        // Генерируем TOML-код для константы или словаря
        if (node->right->type == "Number" || node->right->type == "String" || node->right->type == "Boolean") {
            string constValue = node->right->value;

            if (node->right->type == "String") {
                constValue = "\"" + constValue + "\"";
            }

            // Сохраняем значение константы
            globalSymbols[constName] = constValue;

            tomlCode += constName + " = " + constValue + "\n";
        }
        else if (node->right->type == "Dictionary") {
            // Обрабатываем словарь
            semanticAnalysis(node->right, constName);
        }
        else if (node->right->type == "Reference") {
            // Обработка ссылки на константу
            string refName = node->right->value;
            string constValue = getConstantValue(refName);
            globalSymbols[constName] = constValue;
            tomlCode += constName + " = " + constValue + "\n";
        }
        else {
            semanticError("Недопустимый тип значения в 'set' выражении для '" + constName + "'");
        }
    }
    else if (node->type == "Assignment") {
        // Обработка присваивания переменной значения из константы
        string varName = node->left->value;

        // Проверяем, была ли переменная объявлена ранее
        auto it = globalSymbols.find(varName);
        if (it == globalSymbols.end()) {
            // Если переменная не объявлена, объявляем её как переменную
            declareVariable(varName, "var");
        }
        else {
            if (it->second == "const") {
                semanticError("Константу '" + varName + "' нельзя изменять.");
            }
            // Если переменная уже объявлена как 'var', разрешаем присваивание
        }

        // Проверяем, что значение присваивается из константы
        if (node->right && node->right->type == "Reference") {
            string constName = node->right->value;
            string constValue = getConstantValue(constName);

            // Генерируем TOML-код для переменной
            tomlCode += varName + " = " + constValue + "\n";
        }
        else {
            semanticError("Ожидалось имя константы в правой части присваивания");
        }
    }
    else if (node->type == "Dictionary") {
        // Обработка словаря (таблицы в TOML)
        if (!currentPath.empty()) {
            tomlCode += "[" + currentPath + "]\n";
        }

        // Обработка дочерних узлов словаря
        ASTNode* currentNode = node->left;
        while (currentNode) {
            semanticAnalysis(currentNode, currentPath);
            currentNode = currentNode->right;
        }
    }
    else if (node->type == "Key") {
        // Обработка ключа в словаре
        string keyName = node->value;

        // Полное имя ключа с учетом текущего пути
        string fullKeyName = currentPath.empty() ? keyName : currentPath + "." + keyName;

        // Проверка на повторное объявление ключа
        if (globalSymbols.find(fullKeyName) != globalSymbols.end()) {
            semanticError("Ключ '" + fullKeyName + "' уже объявлен");
        }

        // Обрабатываем значение ключа
        string value;

        if (node->right) {
            if (node->right->type == "String") {
                value = "\"" + node->right->value + "\"";
            }
            else if (node->right->type == "Number") {
                value = node->right->value;
            }
            else if (node->right->type == "Boolean") {
                value = node->right->value;
            }
            else if (node->right->type == "Reference") {
                string constName = node->right->value;
                value = getConstantValue(constName);
            }
            else if (node->right->type == "Dictionary") {
                // Обработка вложенного словаря
                string newPath = fullKeyName;
                semanticAnalysis(node->right, newPath);
                return;
            }
            else {
                semanticError("Недопустимый тип значения для ключа '" + fullKeyName + "'");
            }
        }
        else {
            semanticError("Ключ '" + fullKeyName + "' не имеет значения");
        }

        // Добавляем ключ в глобальную область видимости
        globalSymbols[fullKeyName] = value;

        // Генерируем TOML-код для ключа
        tomlCode += fullKeyName + " = " + value + "\n";
    }
    else if (node->type == "Comment") {
        // Комментарии
        if (node->left->type == "single-line") {
            // Генерируем комментарий в TOML
            tomlCode += "# " + node->left->value + "\n";
        }
        else {
            stringstream ss(node->left->value);
            string line;
            while (getline(ss, line)) {
                tomlCode += "# " + line + "\n";
            }
        }
    }
    else {
        // Обрабатываем остальные узлы
        semanticAnalysis(node->left, currentPath);
        semanticAnalysis(node->right, currentPath);
    }
}

// ОСНОВНАЯ ПРОГРАММА
int main() {
    setlocale(LC_ALL, "Russian");

    string input;
    string line;
    while (getline(cin, line)) {
        if (line == "exit") {
            break;
        }
        input += line + "\n";
    }

    // Лексический анализ
    if (scanner(input, tokens))
        cout << "Лексический анализ кода завершен успешно." << endl;
    printTokens(tokens);

    // Инициализация названий типов токенов
    initializeTokenTypeNames();

    // Синтаксический анализ
    S();
    root->print();
    cout << "Синтаксический анализ кода завершен успешно." << endl;

    // Семантический анализ
    semanticAnalysis(root);
    cout << "Семантический анализ кода завершен успешно." << endl;
    cout << "Сгенерированный TOML-код:" << endl << tomlCode << endl;
    system("pause");
    return 0;
}
