#include "global_symtab.h"

#include "uart.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sancus/sm_support.h>

typedef struct
{
    const char* name;
    void*       value;
    int         is_section;
} Symbol;

#define SYM(name) {#name, name}

static const Symbol symbols[] =
{
    // libc
    SYM(putchar),
    SYM(puts),
    SYM(printf),
    SYM(strlen),
    SYM(exit),

    // SPM support
    SYM(__unprotected_entry),
    SYM(uart_read),
    SYM(uart_read_byte),
    SYM(uart_write),
    SYM(uart_write_byte)
};

#undef SYM

typedef struct SymbolList
{
    Symbol             symbol;
    ElfModule*         owner;
    struct SymbolList* next;
} SymbolList;

static SymbolList* dynamic_symbols_head = NULL;

static int symbol_matches(const Symbol* sym, const char* name)
{
    return !sym->is_section && strcmp(sym->name, name) == 0;
}

void* get_global_symbol_value(const char* name)
{
    unsigned i;
    for (i = 0; i < sizeof(symbols) / sizeof(Symbol); i++)
    {
        const Symbol* sym = &symbols[i];
        if (symbol_matches(sym, name))
            return sym->value;
    }

    SymbolList* current = dynamic_symbols_head;
    while (current != NULL)
    {
        const Symbol* sym = &current->symbol;
        if (symbol_matches(sym, name))
            return sym->value;

        current = current->next;
    }

    return NULL;
}

static int add_symbol(const char* name, void* value, ElfModule* owner,
                      int is_section)
{
    SymbolList** current = &dynamic_symbols_head;
    while (*current != NULL)
        current = &(*current)->next;

    *current = malloc(sizeof(SymbolList));
    if (*current == NULL)
        return 0;

    char* tmp_name = malloc(strlen(name) + 1);
    strcpy(tmp_name, name);
    (*current)->symbol.name = tmp_name;
    (*current)->symbol.value = value;
    (*current)->symbol.is_section = is_section;
    (*current)->owner = owner;
    (*current)->next = NULL;
    return 1;
}

int add_global_symbol(const char* name, void* value, ElfModule* owner)
{
    return add_symbol(name, value, owner, /*is_section=*/0);
}

int add_module_section(const char* name, void* value, ElfModule* owner)
{
    return add_symbol(name, value, owner, /*is_section=*/1);
}

void remove_global_symbols(ElfModule* owner)
{
    SymbolList* prev = NULL;
    SymbolList* current = dynamic_symbols_head;
    while (current != NULL)
    {
        if (current->owner == owner)
        {
            if (prev == NULL)
                dynamic_symbols_head = current->next;
            else
                prev->next = current->next;

            SymbolList* next = current->next;
            free((void*)current->symbol.name);
            free(current);
            current = next;
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }
}

static void print_symbol(const Symbol* sym, print_func pf)
{
    if (sym->is_section)
        pf("%s %p : {}\n", sym->name, sym->value);
    else
        pf("%s = %p;\n", sym->name, sym->value);
}

static void print_symbols(print_func pf, int is_section, ElfModule* module)
{
    if (is_section)
        pf("SECTIONS\n{\n");

    // global symbols don't belong to any module so don't print them if a module
    // is given
    if (module == NULL)
    {
        unsigned i;
        for (i = 0; i < sizeof(symbols) / sizeof(Symbol); i++)
        {
            const Symbol* sym = &symbols[i];
            if (sym->is_section == is_section)
                print_symbol(sym, pf);
        }
    }

    SymbolList* current = dynamic_symbols_head;
    while (current != NULL)
    {
        // if no module is given, all symbols are printed; otherwise only the
        // symbols belonging to the given module are printed
        if (module == NULL || module == current->owner)
        {
            const Symbol* sym = &current->symbol;
            if (sym->is_section == is_section)
                print_symbol(sym, pf);
        }

        current = current->next;
    }

    if (is_section)
        pf("}");

    pf("\n");
}

void print_global_symbols(print_func pf)
{
    print_symbols(pf, /*is_section=*/0, /*module=*/NULL);
}

void print_module_sections(ElfModule* module, print_func pf)
{
    print_symbols(pf, /*is_section=*/1, module);
}
