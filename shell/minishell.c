#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <pwd.h>
#include <sys/types.h>

static char	*built_in_pwd(void);

typedef struct		s_env
{
	char			*var;		// var name exemple : PATH
	struct s_env	*next;
}					t_env;

// First elem dans la liste chainee
static t_env	*first = NULL;

static char	*get_env_var(char *var)
{
	t_env	*tmp = first;
	size_t	len = 0;

	len = strlen(var);

	while (tmp) {
		if (!strncmp(var, tmp->var, len)) {
			return (tmp->var);
		}
		tmp = tmp->next;
	}
	return (NULL);
}


static char	**split(char *raw_cmd, char *limit)
{
	char	*ptr = NULL;
	char	**cmd = NULL;
	size_t	idx = 0;

	// split sur les espaces
	ptr = strtok(raw_cmd, limit);

	while (ptr) {
		cmd = (char **)realloc(cmd, ((idx + 1) * sizeof(char *)));
		cmd[idx] = strdup(ptr);
		ptr = strtok(NULL, limit);
		++idx;
	}
	// On alloue un element qu'on met a NULL a la fin du tableau
	cmd = (char **)realloc(cmd, ((idx + 1) * sizeof(char *)));
	cmd[idx] = NULL;
	return (cmd);
}

static void	free_array(char **array)
{
	for (int i = 0; array[i]; i++) {
		free(array[i]);
		array[i] = NULL;
	}
	free(array);
	array = NULL;
}

static void	exec_cmd(char **cmd, char **env)
{
	pid_t	pid = 0;
	int		status = 0;

	// On fork
	pid = fork();
	if (pid == -1)
		perror("fork");
	// Si le fork a reussit, le processus pere attend l'enfant (process fork)
	else if (pid > 0) {
		// On block le processus parent jusqu'a ce que l'enfant termine puis
		// on kill le processus enfant
		waitpid(pid, &status, 0);
		kill(pid, SIGTERM);
	} else {
		// Le processus enfant execute la commande ou exit si execve echoue
		if (execve(cmd[0], cmd, env) == -1)
			perror("shell");
		exit(EXIT_FAILURE);
	}
}

static bool	get_absolute_path(char **cmd, char **env)
{
	char	*path = NULL;
	char	*bin = NULL;
	char	**path_split = NULL;
	size_t	idx = 0;

	// si cmd n'est pas le chemin absolue, on cherche le chemin absolue du
	// binaire grace a la variable d'environment PATH
	if (cmd[0][0] != '/' && strncmp(cmd[0], "./", 2) != 0) {

		// On cherche la variable d'environemnt PATH si on ne la toruve pas,
		// On ne peux pas concatener alors on quitte la fonction
		for (int i = 0; env[i]; i++) {
			if (!strncmp(env[i], "PATH=", 5)) {
				path = strdup(&env[i][5]);
				break ;
			}
		}
		if (path == NULL)
			return (false);

		// On split le path pour verifier ou ce trouve le binaire
		path_split = split(path, ":");
		free(path);
		path = NULL;

		// On boucle sur chaque dossier du path pour trouver l'emplacement du binaire
		for (idx = 0; path_split[idx]; idx++) {
			// alloc len du path + '/' + len du binaire + 1 pour le '\0'
			bin = (char *)calloc(sizeof(char), (strlen(path_split[idx]) + 1 + strlen(cmd[0]) + 1));
			if (bin == NULL)
				break ;

			// On concat le path , le '/' et le nom du binaire
			strcat(bin, path_split[idx]);
			strcat(bin, "/");
			strcat(bin, cmd[0]);

			// On verfie l'existence du fichier et on quitte la boucle si access
			// renvoi 0
			if (access(bin, F_OK) == 0)
				break ;

			// Nous sommes des gens propre :D
			free(bin);
			bin = NULL;
		}
		free_array(path_split);

		// On remplace le binaire par le path absolue ou NULL si le binaire
		// n'existe pas
		free(cmd[0]);
		cmd[0] = bin;
	} else {
		free(path);
		path = NULL;
	}

	// si le binaire existe on renvoie true
	return (bin == NULL ? false : true);
}

static bool	is_built_in(char *cmd)
{
	const char	*built_in[] = {"pwd", "cd", "env", NULL};

	for (int i = 0; built_in[i]; i++) {
		if (!strcmp(built_in[i], cmd))
			return (true);
	}
	return (false);
}

static void	built_in_cd(char *path)
{
	char	*oldpwd = NULL;
	char	*pwd = NULL;
	char	*pwd_ptr = NULL;

	if (path == NULL)
		return;
	if (chdir(path) == 0) {
		pwd = strrchr(get_env_var("PWD="), '=') + 1;
		oldpwd = strrchr(get_env_var("OLDPWD="), '=') + 1;

		if (oldpwd != NULL && pwd != NULL) {
			strcpy(oldpwd, pwd);
		}
		if (pwd != NULL) {
			pwd = &pwd[-strlen("PWD=")];
			pwd_ptr = built_in_pwd();
			strcpy(pwd, pwd_ptr);
			free(pwd_ptr);
			pwd_ptr = NULL;
		}
	} else {
		perror("chdir");
	}
}

static char	*built_in_pwd(void)
{
	char	*cwd = NULL;

	// On alloue la longueur de PWD= + PATH_MAX + 1 pour le \0
	cwd = (char *)calloc(sizeof(char), PATH_MAX + strlen("PWD=") + 1);
	if (cwd == NULL)
		return (NULL);

	// On concatene le nom de la variable
	strcat(cwd, "PWD=");

	// et on stock le path actuelle apres le = de PATH=
	if (getcwd(&cwd[4], PATH_MAX) == NULL) {
		perror("getcwd()");
	}

	return (cwd);
}

static void	built_in_env(void)
{
	t_env	*tmp = first;

	while (tmp) {
		printf("%s\n", tmp->var);
		tmp = tmp->next;
	}
}

static void	exec_built_in(char **built_in)
{
	if (!strcmp(built_in[0], "pwd"))
		printf("%s\n", get_env_var("PWD="));
	else if (!strcmp(built_in[0], "cd"))
		built_in_cd(built_in[1]);
	else if (!strcmp(built_in[0], "env"))
		built_in_env();
}

static void	add_tail(char *var)
{
	t_env	*ptr = first;
	t_env	*new_node = NULL;

	new_node = (t_env *)calloc(sizeof(t_env), 1);
	if (new_node == NULL) {
		printf("Alloc failure\n");
		return ;
	}

	new_node->var = var;
	new_node->next = NULL;

	if (ptr == NULL) {
		first = new_node;
	} else {
		while (ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = new_node;
	}
}

static void	add_env_var(char *var)
{
	struct passwd	*pw = getpwuid(getuid());
	char			*alloc = NULL;

	if (!strcmp(var, "HOME")) {
		alloc = (char *)calloc(sizeof(char), strlen(pw->pw_dir) + strlen("HOME=") + 1);
		if (alloc == NULL) {
			fprintf(stderr, "Cannot add HOME\n");
			return ;
		}
		strcat(alloc, "HOME=");
		strcat(alloc, pw->pw_dir);
	} else if (!strcmp(var, "PATH")) {
		alloc = strdup("PATH=/bin:/usr/bin");
		if (alloc == NULL) {
			fprintf(stderr, "Cannot add PATH\n");
			return ;
		}
	} else if (!strcmp(var, "OLDPWD")) {
		alloc = strdup("OLDPWD=");
		if (alloc == NULL) {
			fprintf(stderr, "Cannot add OLDPWD\n");
			return ;
		}
	} else if (!strcmp(var, "PWD")) {
		alloc = built_in_pwd();
		if (alloc == NULL) {
			fprintf(stderr, "Cannot add PWD\n");
			return ;
		}
	} else if (!strcmp(var, "SHLVL")) {
		alloc = strdup("SHLVL=1");
		if (alloc == NULL) {
			fprintf(stderr, "Cannot add OLDPWD\n");
			return ;
		}
	}

	add_tail(alloc);
}

static void	dup_env(char **envp)
{
	char	*var_lst[] = {"PATH", "HOME", "OLDPWD", "PWD", "SHLVL", NULL};
	ssize_t	nb_elem = 5; // nombre d'element dasn var_lst

	// boucle sur l'env et stock les variables dans la liste
	for (int i = 0; envp[i]; i++) {
		add_tail(strdup(envp[i]));

		// On verifie que l'on a les variables d'environment minimal
		if (!strncmp(envp[i], "PATH", 4)) var_lst[0] = NULL;
		else if (!strncmp(envp[i], "HOME", 4)) var_lst[1] = NULL;
		else if (!strncmp(envp[i], "OLDPWD", 6)) var_lst[2] = NULL;
		else if (!strncmp(envp[i], "PWD", 3)) var_lst[3] = NULL;
		else if (!strncmp(envp[i], "SHLVL", 5)) var_lst[4] = NULL;
	}

	// On verifie qu l'on a les varaibles PATH, HOME, OLD_PWD et SHLVL
	// sinon on l'ajoute
	for (int i = 0; i < nb_elem; i++) {
		if (var_lst[i] != NULL)
			add_env_var(var_lst[i]);
	}
}

static char	**lst_to_array()
{
	char	**array = NULL;
	t_env	*tmp = first;
	size_t	idx = 0;

	// On compte le nombre d'element dans la liste pour allouer un tableau de pointeurs
	// ou chaque pointeur pointera sur notre environment
	while (tmp) {
		idx++;
		tmp= tmp->next;
	}

	// Allcoation (+ 1 pour l'element null de fin)
	array = (char **)calloc(sizeof(char *), idx + 1);
	if (array == NULL) {
		perror("calloc");
		exit(-1);
	}

	// reset des variables
	tmp = first;
	idx = 0;

	// On fait pointer chaque pointeur sur notrte environment
	while (tmp) {
		array[idx] = tmp->var;
		tmp = tmp->next;
		idx++;
	}

	return (array);
}

static void	free_lst(void)
{
	t_env	*idx = first;
	t_env	*tmp = idx;

	while (idx != NULL) {
		tmp = idx;
		idx = idx->next;
		free(tmp->var);
		tmp->var = NULL;
		free(tmp);
		tmp = NULL;
	}
}

int	main(int argc, char **argv, char **envp)
{
	char	*buffer = NULL;
	size_t	buf_size = 2048;
	char	**cmd = NULL;
	char	**env = NULL;

	dup_env(envp);

	// alloc buffer qui stockera la commande entree par l'user
	buffer = (char *)calloc(sizeof(char), buf_size);
	if (buffer == NULL) {
		perror("Malloc failure");
		return (EXIT_FAILURE);
	}

	// ecriture d'un prompt
	write(1, "$> ", 3);

	// lecture de STDIN en boucle
	while (getline(&buffer, &buf_size, stdin) > 0) {
		cmd = split(buffer, " \n\t");

		if (cmd[0] == NULL)
			fprintf(stderr, "Command not found\n");
		else if (is_built_in(cmd[0]) == true) {
			exec_built_in(cmd);
		} else {
			env = lst_to_array();
			if (get_absolute_path(cmd, env) == true) {
				exec_cmd(cmd, env);
			} else {
				fprintf(stderr, "Command not found\n");
			}
			free(env);
			env = NULL;
		}

		write(1, "$> ", 3);
		free_array(cmd);

	}

	free_lst();
	printf("Aller salut mon pote \n");
	free(buffer);

	(void)argc;
	(void)argv;

	return (0);
}
