#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

static char **split(char *raw_cmd, char *limit)
{
    char *ptr = NULL;
    char **cmd = NULL;
    size_t idx = 0;

    // split sur les espaces
    ptr = strtok(raw_cmd, limit);

    // strtok extrait un à un tous les éléments syntaxiques (tokens) d'une chaîne de caractères
    while (ptr) {
        cmd = (char **)realloc(cmd, ((idx + 1) * sizeof(char *)));
        cmd[idx] = strdup(ptr);
        ptr = strtok(NULL, limit);
        ++idx;
    }

    // alloue un élément que je mets à NULL à la fin du tableau
    cmd = (char **)realloc(cmd, ((idx + 1) * sizeof(char *)));
    cmd[idx] = NULL;
    return cmd;
}

// comme on alloue dynamiquement char **, la fonction "free_array" va permettre de libérer l'allocation
static void free_array(char **array)
{
    for (int i = 0; array[i]; i++) {
        free(array[i]);
        array[i] = NULL;
    }
    free(array);
    array = NULL;
}

static void exec_cmd(char **cmd)
{
    pid_t pid = 0;
    int status = 0;

    // on fork : fork c'est séparer son processus en deux processus identiques
    // Mais il y a un lien de parenté (père/fils) et l'OS n'aime pas que le processus père se termine avant le processus fils
    pid = fork();
    if (pid == -1)
        perror("fork");
    // si le fork a réussi, le processus père attend le fils (processus fork)
    else if (pid > 0) {
        // on bloque le processus père jusqu'à ce que le fils termine, puis
        // on tue le processus fils
        waitpid(pid, &status, 0);
        kill(pid, SIGTERM);
    } else {
        // le processus fils exécute la commande ou exit si execve échoue
        if (execvp(cmd[0], cmd) == -1)
            perror("shell");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    char *buffer = NULL;
    size_t buf_size = 2048;

    // notion de mémoire tampon (buffer)
    // alloc buffer qui stockera la commande entrée par l'utilisateur
    buffer = (char*)calloc(sizeof(char), buf_size);
    if (buffer == NULL) {
        perror("Malloc failure");
        return EXIT_FAILURE;
    }

    // écriture d'un prompt
    write(1, "$>", 3);

    // lecture de STDIN en boucle
    while (getline(&buffer, &buf_size, stdin) > 0) {
        char **cmd = split(buffer, " \n"); // découpe la commande en tokens

        exec_cmd(cmd);

        free_array(cmd); // libère la mémoire allouée pour cmd
        write(1, "$> ", 3); // affiche le prompt
    }

    printf("Bye \n");
    free(buffer);
    return EXIT_SUCCESS;
}
