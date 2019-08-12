#ifndef CONVERSION_H
#define CONVERSION_H

BEGIN_EXTERN_C()

PHPAPI void php_pval_to_variant(pval *pval_arg, VARIANT *var_arg, int codepage TSRMLS_DC);
PHPAPI void php_pval_to_variant_ex(pval *pval_arg, VARIANT *var_arg, pval *pval_type, int codepage TSRMLS_DC);
PHPAPI int php_variant_to_pval(VARIANT *var_arg, pval *pval_arg, int codepage TSRMLS_DC);
PHPAPI OLECHAR *php_char_to_OLECHAR(char *C_str, uint strlen, int codepage TSRMLS_DC);
PHPAPI char *php_OLECHAR_to_char(OLECHAR *unicode_str, uint *out_length, int codepage TSRMLS_DC);

END_EXTERN_C()

#endif