# Threads - Caça-palavras

O objetivo do algoritmo implementado é fazer a busca de uma lista de palavras dentro de um diagrama de letras (matriz de caracteres).

*Regras de negócio:*
- As palavras podem estar em qualquer direção (horizontal, vertical, diagonal e seus sentidos inversos);
- Para cada palavra encontrada, mostrar a posição inicial e a direção;
- Destacar as palavras encontradas no diagrama com letras maiúsculas;
- Usar **threads** para realizar as buscas em paralelo.

## Rodar serviço

- **Buildar executável**
```bash
g++ main.cpp -o cacapalavras -pthread
```

- **Rodar executável**
```bash
./cacapalavra cacapalavras.txt resultado.txt
```

## Lógica por trás do algoritmo 

- No código, o número de threads é igual ao número de palavras a buscar. Esse não é a melhor maneira, pois para listas muito grandes, seria melhor limitar o número de threads ativas simultaneamente (usando um pool de threads, por exemplo).
- **Mutex** (Mutual Exclusion) é um mecanismo usado em programação para controlar o acesso a recursos compartilhados quando várias threads estão executando ao mesmo tempo.

Mutex é uma trava que evita que várias threads mexam ao mesmo tempo em algo que não pode ser compartilhado simultaneamente.