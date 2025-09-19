#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <pthread.h>
#include <utility>
#include <cctype>
#include <chrono>

using namespace std;

// Estrutura para os resultados da busca
struct FoundWord {
    string word;
    int row;
    int col;
    string direction;
};

// Estrutura para os argumentos da thread
struct ThreadArgs {
    string wordToFind;
    int threadId;
};

// Variáveis globais para compartilhar os dados
vector<string> diagram;
vector<string> wordsToFind;
vector<FoundWord> results;

int num_rows, num_cols;
pthread_mutex_t results_mutex;
pthread_mutex_t cout_mutex;

// Direções de busca (8 direções: horizontal, vertical, diagonal)
int dr[] = {0, 0, 1, -1, 1, -1, 1, -1};
int dc[] = {1, -1, 0, 0, 1, -1, -1, 1};
string directions[] = {"direita", "esquerda", "baixo", "cima", "dir/baixo", "esq/cima", "esq/baixo", "dir/cima"};

/**
 * Função para buscar uma palavra em uma direção específica a partir de uma posição
 * @param word: palavra a ser buscada
 * @param start_row: linha inicial
 * @param start_col: coluna inicial
 * @param dir_index: índice da direção (0-7)
 * @return true se a palavra foi encontrada na direção especificada
 */
bool searchDirection(const string& word, int start_row, int start_col, int dir_index) {
    int len = word.length();
    int row = start_row;
    int col = start_col;

    // Verifica se a posição atual está fora dos limites da grade e  
    // Verifica se a letra na posição atual é diferente da letra esperada
    for (int i = 0; i < len; ++i) {
        if (row < 0 || row >= num_rows || col < 0 || col >= num_cols || toupper(diagram[row][col]) != toupper(word[i])) {
            return false;
        }
        row += dr[dir_index];
        col += dc[dir_index];
    }
    return true;
}

/**
 * Função que cada thread executará para buscar uma palavra específica
 * @param args: argumentos da thread contendo a palavra a ser buscada
 */
void* searchWord(void* args) {
    ThreadArgs* arg_struct = (ThreadArgs*) args;
    string word = arg_struct->wordToFind;
    int threadId = arg_struct->threadId;
    bool found = false;

    pthread_mutex_lock(&cout_mutex);
    cout << "[THREAD " << threadId << "] Iniciando busca por: '" << word << "'" << endl;
    pthread_mutex_unlock(&cout_mutex);

    for (int r = 0; r < num_rows; ++r) {                         // percorre cada linha da matriz
        for (int c = 0; c < num_cols; ++c) {                     // percorre cada coluna da matriz
            for (int d = 0; d < 8; ++d) {                        // percorre as 8 direcções possiveis
                if (searchDirection(word, r, c, d)) {
                    // Acesso crítico aos resultados - usar mutex     // mutex+lock vai fazer com que quando uma thread acessar o results_mutex, ele "feche" a variável, impedindo que outras threads mexam nela.
                    pthread_mutex_lock(&results_mutex);
                    results.push_back({word, r + 1, c + 1, directions[d]});   // thread adiciona o reultado na viável results, com a palavra, coordenada e direção.
                    pthread_mutex_unlock(&results_mutex);                     // mutex+inlock vai fazer a thread liberar aquela variável para ser editada por outra.

                    pthread_mutex_lock(&cout_mutex);                  // A mesma coisa ocorre para printar na tela, precisa do lock e unlock
                    cout << "[THREAD " << threadId << "] ENCONTRADA '" << word
                         << "' na posicao (" << (r+1) << "," << (c+1) << ") - " << directions[d] << endl;
                    pthread_mutex_unlock(&cout_mutex);

                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (found) break;
    }

    if (!found) {          // caso não encontra a palavra, segue o mesmo esquema que se achasse, thread vai acessar a variavel, trancar ela e colocar zerado, depois sair e liberar ela.
        pthread_mutex_lock(&results_mutex);
        results.push_back({word, 0, 0, "não encontrada"});
        pthread_mutex_unlock(&results_mutex);

        pthread_mutex_lock(&cout_mutex);
        cout << "[THREAD " << threadId << "] NAO ENCONTRADA: '" << word << "'" << endl;
        pthread_mutex_unlock(&cout_mutex);
    }

    delete arg_struct;
    return NULL;
}

/**
 * Função para ler o arquivo de entrada
 * @param filename: nome do arquivo de entrada
 * @return true se a leitura foi bem-sucedida
 */
bool readInputFile(const string& filename) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Erro: Não foi possível abrir o arquivo de entrada '" << filename << "'" << endl;
        return false;
    }

    inputFile >> num_rows >> num_cols;
    string line;
    getline(inputFile, line);

    cout << "Diagrama: " << num_rows << " linhas x " << num_cols << " colunas" << endl;

    diagram.resize(num_rows);
    for (int i = 0; i < num_rows; ++i) {
        getline(inputFile, line);
        diagram[i] = line;
    }

    while (getline(inputFile, line)) {
        if (!line.empty()) {
            wordsToFind.push_back(line);
        }
    }

    cout << "Palavras a buscar: [";
    for (const auto& w : wordsToFind) {
        cout << w << ", ";
    }
    cout << "]" << endl;

    inputFile.close();
    return true;
}

/**
 * Função para escrever o arquivo de saída
 * @param filename: nome do arquivo de saída
 */
void writeOutputFile(const string& filename) {
    ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        cerr << "Erro: Não foi possível criar o arquivo de saída '" << filename << "'" << endl;
        return;
    }

    vector<string> outputDiagram = diagram;
    for (const auto& res : results) {
        if (res.direction != "não encontrada") {
            int r = res.row - 1;
            int c = res.col - 1;
            int len = res.word.length();

            // Encontra o índice da direção
            int dir_idx = -1;
            for(int i = 0; i < 8; ++i) {
                if (directions[i] == res.direction) {
                    dir_idx = i;
                    break;
                }
            }

            // Marca as letras da palavra encontrada em maiúsculas
            if (dir_idx != -1) {
                for (int i = 0; i < len; ++i) {
                    if(r >=0 && r < num_rows && c >= 0 && c < num_cols){
                        outputDiagram[r][c] = toupper(outputDiagram[r][c]);
                    }
                    r += dr[dir_idx];
                    c += dc[dir_idx];
                }
            }
        }
    }

    for (const auto& row_str : outputDiagram) {
        outputFile << row_str << endl;
    }

    outputFile << endl;

    for (const auto& res : results) {
        outputFile << res.word;
        if (res.direction != "não encontrada") {
            outputFile << " (" << res.row << "," << res.col << "): " << res.direction;
        } else {
            outputFile << ": não encontrada";
        }
        outputFile << endl;
    }

    outputFile.close();
}

/**
 * Função principal
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <arquivo_entrada> <arquivo_saida>" << endl;
        return 1;
    }

    string inputFilename = argv[1];
    string outputFilename = argv[2];

    cout << "==== CACA-PALAVRAS COM THREADS ===" << endl;
    cout << "Arquivo de entrada: " << inputFilename << endl;
    cout << "Arquivo de saIda: " << outputFilename << endl;
    cout << "========================================\n" << endl;

    if (!readInputFile(inputFilename)) {
        return 1;
    }

    pthread_mutex_init(&results_mutex, NULL);
    pthread_mutex_init(&cout_mutex, NULL);

    pthread_t threads[wordsToFind.size()];

    cout << "\nINICIANDO BUSCA (" << wordsToFind.size() << " threads)" << endl;
    cout << "----------------------------------------" << endl;

    auto start = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < wordsToFind.size(); ++i) {
        ThreadArgs* args = new ThreadArgs;
        args->wordToFind = wordsToFind[i];
        args->threadId = i + 1;

        if (pthread_create(&threads[i], NULL, searchWord, args) != 0) {
            cerr << "Erro ao criar thread " << (i+1) << endl;
            return 1;
        }
    }


    for (size_t i = 0; i < wordsToFind.size(); ++i) {
        pthread_join(threads[i], NULL);
    }

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = end - start;

    cout << "----------------------------------------" << endl;
    int encontradas = 0;
    int nao_encontradas = 0;
    cout << "\nRESUMO DOS RESULTADOS:" << endl;
    cout << "=========================" << endl<<endl;

    for (const auto& res : results) {
        if (res.direction != "não encontrada") {
            cout << res.word << " -> (" << res.row << "," << res.col << ") " << res.direction << endl;
            encontradas++;
        } else {
            cout << res.word << " -> nao encontrada" << endl;
            nao_encontradas++;
        }
    }
    cout << "\n=========================" << endl;
    cout << "Total encontradas: " << encontradas << "/" << results.size() << endl;
    cout << "Total nao encontradas: " << nao_encontradas << "/" << results.size() << endl;
    cout << "Tempo de processamento: "
         << elapsed.count() << " segundos" << endl;


    writeOutputFile(outputFilename);

    pthread_mutex_destroy(&results_mutex);
    pthread_mutex_destroy(&cout_mutex);

    cout << "\nResultados salvos em: '" << outputFilename << "'" << endl;
    cout << "========================================" << endl;
    return 0;
}
