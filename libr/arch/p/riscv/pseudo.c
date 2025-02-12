/* radare - LGPL - Copyright 2020-2024 - Aswin C (officialcjunior) */

#include <r_asm.h>

static int replace(int argc, const char *argv[], char *newstr) {
#define MAXPSEUDOOPS 10
	int i, j, k, d;
	char ch;
	struct {
		int narg;
		const char *op;
		const char *str;
		int args[MAXPSEUDOOPS];
	} ops[] = {
		{ 0, "add", "# = # + #", { 1, 2, 3 } },
		{ 0, "addi", "# = # + #", { 1, 2, 3 } },
		{ 0, "and", "# = # & #", { 1, 2, 3 } },
		{ 0, "andi", "# = # & #", { 1, 2, 3 } },
		{ 0, "beq", "if (# == #) goto #", { 1, 2, 3 } },
		{ 0, "auipc", "# = pc + #", {1, 2} },
		{ 0, "bleu", "if (unsigned)# <= # goto #", { 1, 2, 3 } },
		{ 0, "bltu", "if (unsigned)# < # goto #", { 1, 2, 3 } },
		{ 0, "blt", "if (# < #) goto #", { 1, 2, 3 } },
		{ 0, "beqz", "if (# == 0) goto #", { 1, 2 } },
		{ 0, "bne", "if (# != #) goto #", { 1, 2, 3 } },
		{ 0, "bnez", "if (# != 0) goto #", { 1, 2 } },
		{ 0, "bgez", "if (# >= 0) goto #", { 1, 2 } },
		{ 0, "bgtz", "if (# > 0) goto #", { 1, 2 } },
		{ 0, "fld", "# = #", { 1, 2 } },
		{ 0, "j", "jmp #", { 1 } },
		{ 0, "jr", "jmp #", { 1 } },
		{ 0, "jalr", "jmp #", { 1 } },
		{ 0, "jal", "jmp #", { 1 } },
		{ 0, "ld", "# = (double)[#]", { 1, 2 } },
		{ 0, "li", "# = #", { 1, 2 } },
		{ 0, "lh", "# = [#]", { 1, 2 } },
		{ 0, "lui", "# = #", { 1, 2 } },
		{ 0, "lbu", "# = (unsigned)[#]", { 1, 2 } },
		{ 0, "lhu", "# = (unsigned)[#]", { 1, 2 } },
		{ 0, "lw", "# = [#]", { 1, 2 } },
		{ 0, "mv", "# = #", { 1, 2 } },
		{ 0, "or", "# = # | #", { 1, 2, 3 } },
		{ 0, "sd", "[#] = (double)#", { 2, 1 } },
		{ 0, "sw", "[#] = #", { 2, 1 } },
		{ 0, "sb", "[#] = #", { 2, 1 } },
		{ 0, "sh", "[#] = #", { 2, 1 } },
		{ 0, "sub", "# = # - #", { 1, 2, 3 } },
		{ 0, NULL }
	};
	if (!newstr) {
		return false;
	}

	for (i = 0; ops[i].op; i++) {
		if (ops[i].narg) {
			if (argc - 1 != ops[i].narg) {
				continue;
			}
		}
		if (!strcmp (ops[i].op, argv[0])) {
			if (newstr) {
				d = 0;
				j = 0;
				ch = ops[i].str[j];
				for (j = 0, k = 0; ch != '\0'; j++, k++) {
					ch = ops[i].str[j];
					if (ch == '#') {
						if (d >= MAXPSEUDOOPS) {
							// XXX Shouldn't ever happen...
							continue;
						}
						int idx = ops[i].args[d];
						d++;
						if (idx <= 0) {
							// XXX Shouldn't ever happen...
							continue;
						}
						const char *w = argv[idx];
						if (w) {
							strcpy (newstr + k, w);
							k += strlen (w) - 1;
						}
					} else {
						newstr[k] = ch;
					}
				}
				newstr[k] = '\0';
			}
			r_str_replace_char (newstr, '{', '(');
			r_str_replace_char (newstr, '}', ')');
			return true;
		}
	}

	/* TODO: this is slow */
	newstr[0] = '\0';
	for (i = 0; i < argc; i++) {
		strcat (newstr, argv[i]);
		strcat (newstr, (!i || i == argc - 1)? " " : ",");
	}
	r_str_replace_char (newstr, '{', '(');
	r_str_replace_char (newstr, '}', ')');
	return false;
}

static char *patch(RAsmPluginSession *aps, RAnalOp *aop, const char *op) {
	const char *cmd = NULL;
	const int size = aop->size;
	// TODO honor analop->size
	if (!strcmp (op, "nop")) {
		if (size < 2) {
			R_LOG_ERROR ("Can't nop <4 byte instructions");
			return false;
		}
		if (size < 4) {
			cmd = "wx 0100";
		} else {
		       cmd = "wx 13000000";
		}
	} else if (!strcmp (op, "jinf")) {
		if (size < 2) {
			R_LOG_ERROR ("Minimum jinf is 2 byte");
			return false;
		}
		cmd = "wx 01a0";
	}
	return cmd? strdup (cmd): NULL;
}

static char *parse(RAsmPluginSession *aps, const char *data) {
	char w0[256], w1[256], w2[256], w3[256];
	int i, len = strlen (data), n;
	char *ptr, *optr, *num;

	if (len >= sizeof (w0)) {
		return NULL;
	}
	// malloc can be slow here :?
	char *buf = malloc (len + 1);
	if (!buf) {
		return NULL;
	}
	memcpy (buf, data, len + 1);
	char *str = malloc (len + 128);
	strcpy (str, data);
	if (*buf) {
		*w0 = *w1 = *w2 = *w3 = '\0';
		ptr = strchr (buf, ' ');
		if (!ptr) {
			ptr = strchr (buf, '\t');
		}
		if (ptr) {
			*ptr = '\0';
			for (ptr++; *ptr == ' '; ptr++) {
				;
			}
			strncpy (w0, buf, sizeof (w0) - 1);
			strncpy (w1, ptr, sizeof (w1) - 1);

			optr = ptr;
			if (*ptr == '(') {
				ptr = strchr (ptr+1, ')');
			}
			if (ptr && *ptr == '[') {
				ptr = strchr (ptr+1, ']');
			}
			if (ptr && *ptr == '{') {
				ptr = strchr (ptr + 1, '}');
			}
			if (!ptr) {
				eprintf ("Unbalanced bracket\n");
				free (str);
				free (buf);
				return NULL;
			}
			ptr = strchr (ptr, ',');
			if (ptr) {
				*ptr = '\0';
				for (ptr++; *ptr == ' '; ptr++) {
					;
				}
				strncpy (w1, optr, sizeof (w1) - 1);
				strncpy (w2, ptr, sizeof (w2) - 1);
				optr = ptr;
				ptr = strchr (ptr, ',');
				if (ptr) {
					*ptr = '\0';
					for (ptr++; *ptr == ' '; ptr++) {
						;
					}
					strncpy (w2, optr, sizeof (w2) - 1);
					strncpy (w3, ptr, sizeof (w3) - 1);
				}
			}
			ptr = strchr (buf, '(');
			if (ptr) {
				*ptr = 0;
				num = (char*)r_str_lchr (buf, ' ');
				if (!num) {
					num = (char *)r_str_lchr (buf, ',');
				}
				if (num) {
					n = atoi (num + 1);
					*ptr = '[';
					r_str_cpy (num + 1, ptr);
					ptr = (char*)r_str_lchr (buf, ']');
					if (n && ptr) {
						char *rest = strdup (ptr + 1);
						size_t dist = len + 1 - (ptr - buf);
						if (n > 0) {
							snprintf (ptr, dist, "+%d]%s", n, rest);
						} else {
							snprintf (ptr, dist, "%d]%s", n, rest);
						}
						free (rest);
					}
				} else {
					*ptr = '[';
				}
			}
		}
		{
			const char *wa[] = { w0, w1, w2, w3 };
			int nw = 0;
			for (i = 0; i < 4; i++) {
				if (wa[i][0]) {
					nw++;
				}
			}
			replace (nw, wa, str);
		}
	}
	{
		char *s = strdup (str);
		s = r_str_replace (s, "+ -", "- ", 1);
		s = r_str_replace (s, "- -", "+ ", 1);
		strcpy (str, s);
		free (s);
	}
	free (buf);
	return str;
}

RAsmPlugin r_asm_plugin_riscv = {
	.meta = {
		.name = "riscv",
		.desc = "riscv pseudo syntax",
		.author = "pancake",
		.license = "LGPL-3.0-only",
	},
	.parse = parse,
	.patch = patch
};

#ifndef R2_PLUGIN_INCORE
R_API RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_ASM,
	.data = &r_asm_plugin_riscv,
	.version = R2_VERSION};
#endif
