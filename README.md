## Auto-formatting with Emacs and AStyle
1. Add the following code into your .emacs/init.el:
```lisp
(setq c-default-style "bsd"
      c-basic-offset 2)
```
2. Install the astyle program
3. Copy the file "pre-commit" into .git/hooks/
4. Files will now be auto-formatted when running "git commit"
