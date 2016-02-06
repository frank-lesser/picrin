(define-library (picrin repl)
  (import (scheme base)
          (scheme read)
          (scheme write)
          (scheme eval)
          (picrin base))

  (cond-expand
   ((library (picrin readline))
    (import (picrin readline)
            (picrin readline history)))
   (else
    (begin
      (define (readline str)
        (when (tty?)
          (display str)
          (flush-output-port))
        (read-line))
      (define (add-history str)
        #f))))

  (define (init-env)
    (eval
     '(import (scheme base)
              (scheme load)
              (scheme process-context)
              (scheme read)
              (scheme write)
              (scheme file)
              (scheme inexact)
              (scheme cxr)
              (scheme lazy)
              (scheme time)
              (picrin macro))
     (find-library '(picrin user))))

  (define (repl)
    (init-env)
    (let loop ((buf ""))
      (let ((line (readline (if (equal? buf "") "> " ""))))
        (if (eof-object? line)
            (newline)                   ; exit
            (let ((str (string-append buf line "\n")))
              (add-history line)
              (call/cc
               (lambda (exit)
                 (with-exception-handler
                  (lambda (condition)
                    (if (error-object? condition)
                        (unless (equal? (error-object-message condition) "unexpected EOF")
                          (display "error: ")
                          (display (error-object-message condition))
                          (newline)
                          (set! str ""))
                        (begin
                          (display "raised: ")
                          (write condition)
                          (newline)
                          (set! str "")))
                    (exit))
                  (lambda ()
                    (call-with-port (open-input-string str)
                      (lambda (port)
                        (let next ((expr (read port)))
                          (unless (eof-object? expr)
                            (write (eval expr (find-library '(picrin user))))
                            (newline)
                            (set! str "")
                            (next (read port))))))))))
              (loop str))))))

  (export repl))

