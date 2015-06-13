#if 0

=pod
/*
=cut

use strict;

my $src = <<'EOL';

(define-library (picrin base)

  (define-macro call-with-current-environment
    (lambda (form env)
      (list (cadr form) env)))

  (define here
    (call-with-current-environment
     (lambda (env)
       env)))

  (define (the var)                     ; synonym for #'var
    (make-identifier var here))

  (define the-define (the 'define))
  (define the-lambda (the 'lambda))
  (define the-begin (the 'begin))
  (define the-quote (the 'quote))
  (define the-set! (the 'set!))
  (define the-if (the 'if))
  (define the-define-macro (the 'define-macro))

  (define-macro syntax-error
    (lambda (form _)
      (apply error (cdr form))))

  (define-macro define-auxiliary-syntax
    (lambda (form _)
      (define message
        (string-append
         "invalid use of auxiliary syntax: '" (symbol->string (cadr form)) "'"))
      (list
       the-define-macro
       (cadr form)
       (list the-lambda '_
             (list (the 'error) message)))))

  (define-auxiliary-syntax else)
  (define-auxiliary-syntax =>)
  (define-auxiliary-syntax unquote)
  (define-auxiliary-syntax unquote-splicing)
  (define-auxiliary-syntax syntax-unquote)
  (define-auxiliary-syntax syntax-unquote-splicing)

  (define-macro let
    (lambda (form env)
      (if (variable? (cadr form))
          (list
           (list the-lambda '()
                 (list the-define (cadr form)
                       (cons the-lambda
                             (cons (map car (car (cddr form)))
                                   (cdr (cddr form)))))
                 (cons (cadr form) (map cadr (car (cddr form))))))
          (cons
           (cons
            the-lambda
            (cons (map car (cadr form))
                  (cddr form)))
           (map cadr (cadr form))))))

  (define-macro and
    (lambda (form env)
      (if (null? (cdr form))
          #t
          (list the-if
                (cadr form)
                (cons (the 'and) (cddr form))
                #f))))

  (define-macro or
    (lambda (form env)
      (if (null? (cdr form))
          #t
          (let ((tmp (make-identifier 'it env)))
            (list (the 'let)
                  (list (list tmp (cadr form)))
                  (list the-if
                        tmp
                        tmp
                        (cons (the 'or) (cddr form))))))))

  (define-macro cond
    (lambda (form env)
      (let ((clauses (cdr form)))
        (if (null? clauses)
            #undefined
            (let ((clause (car clauses)))
              (if (and (variable? (car clause))
                       (variable=? (the 'else) (make-identifier (car clause) env)))
                  (cons the-begin (cdr clause))
                  (if (and (variable? (cadr clause))
                           (variable=? (the '=>) (make-identifier (cadr clause) env)))
                      (let ((tmp (make-identifier 'tmp here)))
                        (list (the 'let) (list (list tmp (car clause)))
                              (list the-if tmp
                                    (list (car (cddr clause)) tmp)
                                    (cons (the 'cond) (cdr clauses)))))
                      (list the-if (car clause)
                            (cons the-begin (cdr clause))
                            (cons (the 'cond) (cdr clauses))))))))))

  (define-macro quasiquote
    (lambda (form env)

      (define (quasiquote? form)
        (and (pair? form)
             (variable? (car form))
             (variable=? (the 'quasiquote) (make-identifier (car form) env))))

      (define (unquote? form)
        (and (pair? form)
             (variable? (car form))
             (variable=? (the 'unquote) (make-identifier (car form) env))))

      (define (unquote-splicing? form)
        (and (pair? form)
             (pair? (car form))
             (variable? (caar form))
             (variable=? (the 'unquote-splicing) (make-identifier (caar form) env))))

      (define (qq depth expr)
        (cond
         ;; unquote
         ((unquote? expr)
          (if (= depth 1)
              (car (cdr expr))
              (list (the 'list)
                    (list (the 'quote) (the 'unquote))
                    (qq (- depth 1) (car (cdr expr))))))
         ;; unquote-splicing
         ((unquote-splicing? expr)
          (if (= depth 1)
              (list (the 'append)
                    (car (cdr (car expr)))
                    (qq depth (cdr expr)))
              (list (the 'cons)
                    (list (the 'list)
                          (list (the 'quote) (the 'unquote-splicing))
                          (qq (- depth 1) (car (cdr (car expr)))))
                    (qq depth (cdr expr)))))
         ;; quasiquote
         ((quasiquote? expr)
          (list (the 'list)
                (list (the 'quote) (the 'quasiquote))
                (qq (+ depth 1) (car (cdr expr)))))
         ;; list
         ((pair? expr)
          (list (the 'cons)
                (qq depth (car expr))
                (qq depth (cdr expr))))
         ;; vector
         ((vector? expr)
          (list (the 'list->vector) (qq depth (vector->list expr))))
         ;; simple datum
         (else
          (list (the 'quote) expr))))

      (let ((x (cadr form)))
        (qq 1 x))))

  (define-macro let*
    (lambda (form env)
      (let ((bindings (car (cdr form)))
            (body     (cdr (cdr form))))
        (if (null? bindings)
            `(,(the 'let) () ,@body)
            `(,(the 'let) ((,(car (car bindings)) ,@(cdr (car bindings))))
              (,(the 'let*) (,@(cdr bindings))
               ,@body))))))

  (define-macro letrec
    (lambda (form env)
      `(,(the 'letrec*) ,@(cdr form))))

  (define-macro letrec*
    (lambda (form env)
      (let ((bindings (car (cdr form)))
            (body     (cdr (cdr form))))
        (let ((variables (map (lambda (v) `(,v #f)) (map car bindings)))
              (initials  (map (lambda (v) `(,(the 'set!) ,@v)) bindings)))
          `(,(the 'let) (,@variables)
            ,@initials
            ,@body)))))

  (define-macro let-values
    (lambda (form env)
      `(,(the 'let*-values) ,@(cdr form))))

  (define-macro let*-values
    (lambda (form env)
      (let ((formal (car (cdr form)))
            (body   (cdr (cdr form))))
        (if (null? formal)
            `(,(the 'let) () ,@body)
            `(,(the 'call-with-values) (,the-lambda () ,@(cdr (car formal)))
              (,(the 'lambda) (,@(car (car formal)))
               (,(the 'let*-values) (,@(cdr formal))
                ,@body)))))))

  (define-macro define-values
    (lambda (form env)
      (let ((formal (car (cdr form)))
            (body   (cdr (cdr form))))
        (let ((arguments (make-identifier 'arguments here)))
          `(,the-begin
            ,@(let loop ((formal formal))
                (if (pair? formal)
                    `((,the-define ,(car formal) #undefined) ,@(loop (cdr formal)))
                    (if (variable? formal)
                        `((,the-define ,formal #undefined))
                        '())))
            (,(the 'call-with-values) (,the-lambda () ,@body)
             (,the-lambda
              ,arguments
              ,@(let loop ((formal formal) (args arguments))
                  (if (pair? formal)
                      `((,the-set! ,(car formal) (,(the 'car) ,args)) ,@(loop (cdr formal) `(,(the 'cdr) ,args)))
                      (if (variable? formal)
                          `((,the-set! ,formal ,args))
                          '()))))))))))

  (define-macro do
    (lambda (form env)
      (let ((bindings (car (cdr form)))
            (test     (car (car (cdr (cdr form)))))
            (cleanup  (car (cdr (cdr form))))
            (body     (cdr (cdr (cdr form)))))
        (let ((loop (make-identifier 'loop here)))
          `(,(the 'let) ,loop ,(map (lambda (x) (list (car x) (car (cdr x)))) bindings)
            (,the-if ,test
             (,the-begin
              ,cleanup)
             (,the-begin
              ,@body
              (,loop ,@(map (lambda (x) (if (null? (cdr (cdr x))) (car x) (car (cddr x)))) bindings)))))))))

  (define-macro when
    (lambda (form env)
      (let ((test (car (cdr form)))
            (body (cdr (cdr form))))
        `(,the-if ,test
                  (,the-begin ,@body)
                  #undefined))))

  (define-macro unless
    (lambda (form env)
      (let ((test (car (cdr form)))
            (body (cdr (cdr form))))
        `(,the-if ,test
                  #undefined
                  (,the-begin ,@body)))))

  (define-macro case
    (lambda (form env)
      (let ((key     (car (cdr form)))
            (clauses (cdr (cdr form))))
        (let ((the-key (make-identifier 'key here)))
          `(,(the 'let) ((,the-key ,key))
            ,(let loop ((clauses clauses))
               (if (null? clauses)
                   #undefined
                   (let ((clause (car clauses)))
                     `(,the-if ,(if (and (variable? (car clause))
                                         (variable=? (the 'else) (make-identifier (car clause) env)))
                                    #t
                                    `(,(the 'or) ,@(map (lambda (x) `(,(the 'eqv?) ,the-key (,the-quote ,x))) (car clause))))
                               ,(if (and (variable? (car clause))
                                         (variable=? (the '=>) (make-identifier (car clause) env)))
                                    `(,(car (cdr clause)) ,the-key)
                                    `(,the-begin ,@(cdr clause)))
                               ,(loop (cdr clauses)))))))))))

  (define-macro parameterize
    (lambda (form env)
      (let ((formal (car (cdr form)))
            (body   (cdr (cdr form))))
        `(,(the 'with-parameter)
          (,(the 'lambda) ()
           ,@formal
           ,@body)))))

  (define-macro syntax-quote
    (lambda (form env)
      (letrec
          ((wrap (let ((register (make-register)))
                   (lambda (var)
                     (let ((id (register var)))
                       (if (undefined? id)
                           (let ((id (make-identifier var env)))
                             (register var id)
                             id)
                           id)))))
           (walk (lambda (f form)
                   (cond
                    ((variable? form)
                     (f form))
                    ((pair? form)
                     (cons (walk f (car form)) (walk f (cdr form))))
                    ((vector? form)
                     (list->vector (walk f (vector->list form))))
                    (else
                     form)))))
        (list the-quote (walk wrap (cadr form))))))

  (define-macro syntax-quasiquote
    (lambda (form env)
      (letrec
          ((wrap (let ((register (make-register)))
                   (lambda (var)
                     (let ((id (register var)))
                       (if (undefined? id)
                           (let ((id (make-identifier var env)))
                             (register var id)
                             id)
                           id))))))

        (define (syntax-quasiquote? form)
          (and (pair? form)
               (variable? (car form))
               (variable=? (the 'syntax-quasiquote) (make-identifier (car form) env))))

        (define (syntax-unquote? form)
          (and (pair? form)
               (variable? (car form))
               (variable=? (the 'syntax-unquote) (make-identifier (car form) env))))

        (define (syntax-unquote-splicing? form)
          (and (pair? form)
               (pair? (car form))
               (variable? (caar form))
               (variable=? (the 'syntax-unquote-splicing) (make-identifier (caar form) env))))

        (define (qq depth expr)
          (cond
           ;; syntax-unquote
           ((syntax-unquote? expr)
            (if (= depth 1)
                (car (cdr expr))
                (list (the 'list)
                      (list (the 'quote) (the 'syntax-unquote))
                      (qq (- depth 1) (car (cdr expr))))))
           ;; syntax-unquote-splicing
           ((syntax-unquote-splicing? expr)
            (if (= depth 1)
                (list (the 'append)
                      (car (cdr (car expr)))
                      (qq depth (cdr expr)))
                (list (the 'cons)
                      (list (the 'list)
                            (list (the 'quote) (the 'syntax-unquote-splicing))
                            (qq (- depth 1) (car (cdr (car expr)))))
                      (qq depth (cdr expr)))))
           ;; syntax-quasiquote
           ((syntax-quasiquote? expr)
            (list (the 'list)
                  (list (the 'quote) (the 'quasiquote))
                  (qq (+ depth 1) (car (cdr expr)))))
           ;; list
           ((pair? expr)
            (list (the 'cons)
                  (qq depth (car expr))
                  (qq depth (cdr expr))))
           ;; vector
           ((vector? expr)
            (list (the 'list->vector) (qq depth (vector->list expr))))
           ;; variable
           ((variable? expr)
            (list (the 'quote) (wrap expr)))
           ;; simple datum
           (else
            (list (the 'quote) expr))))

        (let ((x (cadr form)))
          (qq 1 x)))))

  (define (transformer f)
    (lambda (form env)
      (let ((register1 (make-register))
            (register2 (make-register)))
        (letrec
            ((wrap (lambda (var1)
                     (let ((var2 (register1 var1)))
                       (if (undefined? var2)
                           (let ((var2 (make-identifier var1 env)))
                             (register1 var1 var2)
                             (register2 var2 var1)
                             var2)
                           var2))))
             (unwrap (lambda (var2)
                       (let ((var1 (register2 var2)))
                         (if (undefined? var1)
                             var2
                             var1))))
             (walk (lambda (f form)
                     (cond
                      ((variable? form)
                       (f form))
                      ((pair? form)
                       (cons (walk f (car form)) (walk f (cdr form))))
                      ((vector? form)
                       (list->vector (walk f (vector->list form))))
                      (else
                       form)))))
          (let ((form (cdr form)))
            (walk unwrap (apply f (walk wrap form))))))))

  (define-macro define-syntax
    (lambda (form env)
      (let ((formal (car (cdr form)))
            (body   (cdr (cdr form))))
        (if (pair? formal)
            `(,(the 'define-syntax) ,(car formal) (,the-lambda ,(cdr formal) ,@body))
            `(,the-define-macro ,formal (,(the 'transformer) (,the-begin ,@body)))))))

  (define-macro letrec-syntax
    (lambda (form env)
      (let ((formal (car (cdr form)))
            (body   (cdr (cdr form))))
        `(let ()
           ,@(map (lambda (x)
                    `(,(the 'define-syntax) ,(car x) ,(cadr x)))
                  formal)
           ,@body))))

  (define-macro let-syntax
    (lambda (form env)
      `(,(the 'letrec-syntax) ,@(cdr form))))

  (export let let* letrec letrec*
          let-values let*-values define-values
          quasiquote unquote unquote-splicing
          and or
          cond case else =>
          do when unless
          parameterize
          define-syntax
          syntax-quote syntax-unquote
          syntax-quasiquote syntax-unquote-splicing
          let-syntax letrec-syntax
          syntax-error))

EOL

open IN, "./boot.c";
my @data = <IN>;
close IN;

open STDOUT, ">", "./boot.c";

foreach (@data) {
  print;
  last if $_ eq "#---END---\n";
}

print "\n#endif\n\n";

print <<EOL;
const char pic_boot[][80] = {
EOL

my @lines = $src =~ /.{0,80}/gs;

foreach (@lines) {
  s/\\/\\\\/g;
  s/"/\\"/g;
  s/\n/\\n/g;
  print "\"$_\",\n";
}
print "\"\"\n";

=pod
*/
=cut

print <<EOL;
};

#if 0
Local Variables:
mode: scheme
End:

=cut
#endif
EOL

=pod

#---END---

#endif

const char pic_boot[][80] = {
"\n(define-library (picrin base)\n\n  (define-macro call-with-current-environment\n  ",
"  (lambda (form env)\n      (list (cadr form) env)))\n\n  (define here\n    (call-wi",
"th-current-environment\n     (lambda (env)\n       env)))\n\n  (define (the var)    ",
"                 ; synonym for #'var\n    (make-identifier var here))\n\n  (define ",
"the-define (the 'define))\n  (define the-lambda (the 'lambda))\n  (define the-begi",
"n (the 'begin))\n  (define the-quote (the 'quote))\n  (define the-set! (the 'set!)",
")\n  (define the-if (the 'if))\n  (define the-define-macro (the 'define-macro))\n\n ",
" (define-macro syntax-error\n    (lambda (form _)\n      (apply error (cdr form)))",
")\n\n  (define-macro define-auxiliary-syntax\n    (lambda (form _)\n      (define me",
"ssage\n        (string-append\n         \"invalid use of auxiliary syntax: '\" (symb",
"ol->string (cadr form)) \"'\"))\n      (list\n       the-define-macro\n       (cadr f",
"orm)\n       (list the-lambda '_\n             (list (the 'error) message)))))\n\n  ",
"(define-auxiliary-syntax else)\n  (define-auxiliary-syntax =>)\n  (define-auxiliar",
"y-syntax unquote)\n  (define-auxiliary-syntax unquote-splicing)\n  (define-auxilia",
"ry-syntax syntax-unquote)\n  (define-auxiliary-syntax syntax-unquote-splicing)\n\n ",
" (define-macro let\n    (lambda (form env)\n      (if (variable? (cadr form))\n    ",
"      (list\n           (list the-lambda '()\n                 (list the-define (c",
"adr form)\n                       (cons the-lambda\n                             (",
"cons (map car (car (cddr form)))\n                                   (cdr (cddr f",
"orm)))))\n                 (cons (cadr form) (map cadr (car (cddr form))))))\n    ",
"      (cons\n           (cons\n            the-lambda\n            (cons (map car (",
"cadr form))\n                  (cddr form)))\n           (map cadr (cadr form)))))",
")\n\n  (define-macro and\n    (lambda (form env)\n      (if (null? (cdr form))\n     ",
"     #t\n          (list the-if\n                (cadr form)\n                (cons",
" (the 'and) (cddr form))\n                #f))))\n\n  (define-macro or\n    (lambda ",
"(form env)\n      (if (null? (cdr form))\n          #t\n          (let ((tmp (make-",
"identifier 'it env)))\n            (list (the 'let)\n                  (list (list",
" tmp (cadr form)))\n                  (list the-if\n                        tmp\n  ",
"                      tmp\n                        (cons (the 'or) (cddr form))))",
"))))\n\n  (define-macro cond\n    (lambda (form env)\n      (let ((clauses (cdr form",
")))\n        (if (null? clauses)\n            #undefined\n            (let ((clause",
" (car clauses)))\n              (if (and (variable? (car clause))\n               ",
"        (variable=? (the 'else) (make-identifier (car clause) env)))\n           ",
"       (cons the-begin (cdr clause))\n                  (if (and (variable? (cadr",
" clause))\n                           (variable=? (the '=>) (make-identifier (cad",
"r clause) env)))\n                      (let ((tmp (make-identifier 'tmp here)))\n",
"                        (list (the 'let) (list (list tmp (car clause)))\n        ",
"                      (list the-if tmp\n                                    (list",
" (car (cddr clause)) tmp)\n                                    (cons (the 'cond) ",
"(cdr clauses)))))\n                      (list the-if (car clause)\n              ",
"              (cons the-begin (cdr clause))\n                            (cons (t",
"he 'cond) (cdr clauses))))))))))\n\n  (define-macro quasiquote\n    (lambda (form e",
"nv)\n\n      (define (quasiquote? form)\n        (and (pair? form)\n             (va",
"riable? (car form))\n             (variable=? (the 'quasiquote) (make-identifier ",
"(car form) env))))\n\n      (define (unquote? form)\n        (and (pair? form)\n    ",
"         (variable? (car form))\n             (variable=? (the 'unquote) (make-id",
"entifier (car form) env))))\n\n      (define (unquote-splicing? form)\n        (and",
" (pair? form)\n             (pair? (car form))\n             (variable? (caar form",
"))\n             (variable=? (the 'unquote-splicing) (make-identifier (caar form)",
" env))))\n\n      (define (qq depth expr)\n        (cond\n         ;; unquote\n      ",
"   ((unquote? expr)\n          (if (= depth 1)\n              (car (cdr expr))\n   ",
"           (list (the 'list)\n                    (list (the 'quote) (the 'unquot",
"e))\n                    (qq (- depth 1) (car (cdr expr))))))\n         ;; unquote",
"-splicing\n         ((unquote-splicing? expr)\n          (if (= depth 1)\n         ",
"     (list (the 'append)\n                    (car (cdr (car expr)))\n            ",
"        (qq depth (cdr expr)))\n              (list (the 'cons)\n                 ",
"   (list (the 'list)\n                          (list (the 'quote) (the 'unquote-",
"splicing))\n                          (qq (- depth 1) (car (cdr (car expr)))))\n  ",
"                  (qq depth (cdr expr)))))\n         ;; quasiquote\n         ((qua",
"siquote? expr)\n          (list (the 'list)\n                (list (the 'quote) (t",
"he 'quasiquote))\n                (qq (+ depth 1) (car (cdr expr)))))\n         ;;",
" list\n         ((pair? expr)\n          (list (the 'cons)\n                (qq dep",
"th (car expr))\n                (qq depth (cdr expr))))\n         ;; vector\n      ",
"   ((vector? expr)\n          (list (the 'list->vector) (qq depth (vector->list e",
"xpr))))\n         ;; simple datum\n         (else\n          (list (the 'quote) exp",
"r))))\n\n      (let ((x (cadr form)))\n        (qq 1 x))))\n\n  (define-macro let*\n  ",
"  (lambda (form env)\n      (let ((bindings (car (cdr form)))\n            (body  ",
"   (cdr (cdr form))))\n        (if (null? bindings)\n            `(,(the 'let) () ",
",@body)\n            `(,(the 'let) ((,(car (car bindings)) ,@(cdr (car bindings))",
"))\n              (,(the 'let*) (,@(cdr bindings))\n               ,@body))))))\n\n ",
" (define-macro letrec\n    (lambda (form env)\n      `(,(the 'letrec*) ,@(cdr form",
"))))\n\n  (define-macro letrec*\n    (lambda (form env)\n      (let ((bindings (car ",
"(cdr form)))\n            (body     (cdr (cdr form))))\n        (let ((variables (",
"map (lambda (v) `(,v #f)) (map car bindings)))\n              (initials  (map (la",
"mbda (v) `(,(the 'set!) ,@v)) bindings)))\n          `(,(the 'let) (,@variables)\n",
"            ,@initials\n            ,@body)))))\n\n  (define-macro let-values\n    (",
"lambda (form env)\n      `(,(the 'let*-values) ,@(cdr form))))\n\n  (define-macro l",
"et*-values\n    (lambda (form env)\n      (let ((formal (car (cdr form)))\n        ",
"    (body   (cdr (cdr form))))\n        (if (null? formal)\n            `(,(the 'l",
"et) () ,@body)\n            `(,(the 'call-with-values) (,the-lambda () ,@(cdr (ca",
"r formal)))\n              (,(the 'lambda) (,@(car (car formal)))\n               ",
"(,(the 'let*-values) (,@(cdr formal))\n                ,@body)))))))\n\n  (define-m",
"acro define-values\n    (lambda (form env)\n      (let ((formal (car (cdr form)))\n",
"            (body   (cdr (cdr form))))\n        (let ((arguments (make-identifier",
" 'arguments here)))\n          `(,the-begin\n            ,@(let loop ((formal form",
"al))\n                (if (pair? formal)\n                    `((,the-define ,(car",
" formal) #undefined) ,@(loop (cdr formal)))\n                    (if (variable? f",
"ormal)\n                        `((,the-define ,formal #undefined))\n             ",
"           '())))\n            (,(the 'call-with-values) (,the-lambda () ,@body)\n",
"             (,the-lambda\n              ,arguments\n              ,@(let loop ((f",
"ormal formal) (args arguments))\n                  (if (pair? formal)\n           ",
"           `((,the-set! ,(car formal) (,(the 'car) ,args)) ,@(loop (cdr formal) ",
"`(,(the 'cdr) ,args)))\n                      (if (variable? formal)\n            ",
"              `((,the-set! ,formal ,args))\n                          '()))))))))",
"))\n\n  (define-macro do\n    (lambda (form env)\n      (let ((bindings (car (cdr fo",
"rm)))\n            (test     (car (car (cdr (cdr form)))))\n            (cleanup  ",
"(car (cdr (cdr form))))\n            (body     (cdr (cdr (cdr form)))))\n        (",
"let ((loop (make-identifier 'loop here)))\n          `(,(the 'let) ,loop ,(map (l",
"ambda (x) (list (car x) (car (cdr x)))) bindings)\n            (,the-if ,test\n   ",
"          (,the-begin\n              ,cleanup)\n             (,the-begin\n         ",
"     ,@body\n              (,loop ,@(map (lambda (x) (if (null? (cdr (cdr x))) (c",
"ar x) (car (cddr x)))) bindings)))))))))\n\n  (define-macro when\n    (lambda (form",
" env)\n      (let ((test (car (cdr form)))\n            (body (cdr (cdr form))))\n ",
"       `(,the-if ,test\n                  (,the-begin ,@body)\n                  #",
"undefined))))\n\n  (define-macro unless\n    (lambda (form env)\n      (let ((test (",
"car (cdr form)))\n            (body (cdr (cdr form))))\n        `(,the-if ,test\n  ",
"                #undefined\n                  (,the-begin ,@body)))))\n\n  (define-",
"macro case\n    (lambda (form env)\n      (let ((key     (car (cdr form)))\n       ",
"     (clauses (cdr (cdr form))))\n        (let ((the-key (make-identifier 'key he",
"re)))\n          `(,(the 'let) ((,the-key ,key))\n            ,(let loop ((clauses",
" clauses))\n               (if (null? clauses)\n                   #undefined\n    ",
"               (let ((clause (car clauses)))\n                     `(,the-if ,(if",
" (and (variable? (car clause))\n                                         (variabl",
"e=? (the 'else) (make-identifier (car clause) env)))\n                           ",
"         #t\n                                    `(,(the 'or) ,@(map (lambda (x) ",
"`(,(the 'eqv?) ,the-key (,the-quote ,x))) (car clause))))\n                      ",
"         ,(if (and (variable? (car clause))\n                                    ",
"     (variable=? (the '=>) (make-identifier (car clause) env)))\n                ",
"                    `(,(car (cdr clause)) ,the-key)\n                            ",
"        `(,the-begin ,@(cdr clause)))\n                               ,(loop (cdr",
" clauses)))))))))))\n\n  (define-macro parameterize\n    (lambda (form env)\n      (",
"let ((formal (car (cdr form)))\n            (body   (cdr (cdr form))))\n        `(",
",(the 'with-parameter)\n          (,(the 'lambda) ()\n           ,@formal\n        ",
"   ,@body)))))\n\n  (define-macro syntax-quote\n    (lambda (form env)\n      (letre",
"c\n          ((wrap (let ((register (make-register)))\n                   (lambda ",
"(var)\n                     (let ((id (register var)))\n                       (if",
" (undefined? id)\n                           (let ((id (make-identifier var env))",
")\n                             (register var id)\n                             id",
")\n                           id)))))\n           (walk (lambda (f form)\n         ",
"          (cond\n                    ((variable? form)\n                     (f fo",
"rm))\n                    ((pair? form)\n                     (cons (walk f (car f",
"orm)) (walk f (cdr form))))\n                    ((vector? form)\n                ",
"     (list->vector (walk f (vector->list form))))\n                    (else\n    ",
"                 form)))))\n        (list the-quote (walk wrap (cadr form))))))\n\n",
"  (define-macro syntax-quasiquote\n    (lambda (form env)\n      (letrec\n         ",
" ((wrap (let ((register (make-register)))\n                   (lambda (var)\n     ",
"                (let ((id (register var)))\n                       (if (undefined",
"? id)\n                           (let ((id (make-identifier var env)))\n         ",
"                    (register var id)\n                             id)\n         ",
"                  id))))))\n\n        (define (syntax-quasiquote? form)\n          ",
"(and (pair? form)\n               (variable? (car form))\n               (variable",
"=? (the 'syntax-quasiquote) (make-identifier (car form) env))))\n\n        (define",
" (syntax-unquote? form)\n          (and (pair? form)\n               (variable? (c",
"ar form))\n               (variable=? (the 'syntax-unquote) (make-identifier (car",
" form) env))))\n\n        (define (syntax-unquote-splicing? form)\n          (and (",
"pair? form)\n               (pair? (car form))\n               (variable? (caar fo",
"rm))\n               (variable=? (the 'syntax-unquote-splicing) (make-identifier ",
"(caar form) env))))\n\n        (define (qq depth expr)\n          (cond\n           ",
";; syntax-unquote\n           ((syntax-unquote? expr)\n            (if (= depth 1)",
"\n                (car (cdr expr))\n                (list (the 'list)\n            ",
"          (list (the 'quote) (the 'syntax-unquote))\n                      (qq (-",
" depth 1) (car (cdr expr))))))\n           ;; syntax-unquote-splicing\n           ",
"((syntax-unquote-splicing? expr)\n            (if (= depth 1)\n                (li",
"st (the 'append)\n                      (car (cdr (car expr)))\n                  ",
"    (qq depth (cdr expr)))\n                (list (the 'cons)\n                   ",
"   (list (the 'list)\n                            (list (the 'quote) (the 'syntax",
"-unquote-splicing))\n                            (qq (- depth 1) (car (cdr (car e",
"xpr)))))\n                      (qq depth (cdr expr)))))\n           ;; syntax-qua",
"siquote\n           ((syntax-quasiquote? expr)\n            (list (the 'list)\n    ",
"              (list (the 'quote) (the 'quasiquote))\n                  (qq (+ dep",
"th 1) (car (cdr expr)))))\n           ;; list\n           ((pair? expr)\n          ",
"  (list (the 'cons)\n                  (qq depth (car expr))\n                  (q",
"q depth (cdr expr))))\n           ;; vector\n           ((vector? expr)\n          ",
"  (list (the 'list->vector) (qq depth (vector->list expr))))\n           ;; varia",
"ble\n           ((variable? expr)\n            (list (the 'quote) (wrap expr)))\n  ",
"         ;; simple datum\n           (else\n            (list (the 'quote) expr)))",
")\n\n        (let ((x (cadr form)))\n          (qq 1 x)))))\n\n  (define (transformer",
" f)\n    (lambda (form env)\n      (let ((register1 (make-register))\n            (",
"register2 (make-register)))\n        (letrec\n            ((wrap (lambda (var1)\n  ",
"                   (let ((var2 (register1 var1)))\n                       (if (un",
"defined? var2)\n                           (let ((var2 (make-identifier var1 env)",
"))\n                             (register1 var1 var2)\n                          ",
"   (register2 var2 var1)\n                             var2)\n                    ",
"       var2))))\n             (unwrap (lambda (var2)\n                       (let ",
"((var1 (register2 var2)))\n                         (if (undefined? var1)\n       ",
"                      var2\n                             var1))))\n             (w",
"alk (lambda (f form)\n                     (cond\n                      ((variable",
"? form)\n                       (f form))\n                      ((pair? form)\n   ",
"                    (cons (walk f (car form)) (walk f (cdr form))))\n            ",
"          ((vector? form)\n                       (list->vector (walk f (vector->",
"list form))))\n                      (else\n                       form)))))\n     ",
"     (let ((form (cdr form)))\n            (walk unwrap (apply f (walk wrap form)",
")))))))\n\n  (define-macro define-syntax\n    (lambda (form env)\n      (let ((forma",
"l (car (cdr form)))\n            (body   (cdr (cdr form))))\n        (if (pair? fo",
"rmal)\n            `(,(the 'define-syntax) ,(car formal) (,the-lambda ,(cdr forma",
"l) ,@body))\n            `(,the-define-macro ,formal (,(the 'transformer) (,the-b",
"egin ,@body)))))))\n\n  (define-macro letrec-syntax\n    (lambda (form env)\n      (",
"let ((formal (car (cdr form)))\n            (body   (cdr (cdr form))))\n        `(",
"let ()\n           ,@(map (lambda (x)\n                    `(,(the 'define-syntax)",
" ,(car x) ,(cadr x)))\n                  formal)\n           ,@body))))\n\n  (define",
"-macro let-syntax\n    (lambda (form env)\n      `(,(the 'letrec-syntax) ,@(cdr fo",
"rm))))\n\n  (export let let* letrec letrec*\n          let-values let*-values defin",
"e-values\n          quasiquote unquote unquote-splicing\n          and or\n        ",
"  cond case else =>\n          do when unless\n          parameterize\n          de",
"fine-syntax\n          syntax-quote syntax-unquote\n          syntax-quasiquote sy",
"ntax-unquote-splicing\n          let-syntax letrec-syntax\n          syntax-error)",
")\n\n",
"",
""
};

#if 0
Local Variables:
mode: scheme
End:

=cut
#endif
