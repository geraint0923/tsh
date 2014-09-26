"if !empty($GOROOT)
"  set rtp+=$GOROOT/misc/vim
"elseif isdirectory('$HOME/.usr/local/go')
"  set rtp+=$HOME/.usr/local/go/misc/vim
"end


set nu
set autoindent
set smartindent
set autochdir
set smarttab
set noexpandtab
set tabstop=8
set cindent
set shiftwidth=8
set softtabstop=8
set backspace=indent,eol,start

set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
" alternatively, pass a path where Vundle should install plugins
"call vundle#begin('~/some/path/here')

" let Vundle manage Vundle, required
Plugin 'gmarik/Vundle.vim'
Plugin 'Valloric/YouCompleteMe'
Plugin 'SirVer/ultisnips'
filetype plugin indent on
syntax on

call vundle#end()            " required
filetype plugin indent on    " required

let g:ycm_global_ycm_extra_conf = expand("~/.vim/.ycm_extra_conf.py")
let g:ycm_confirm_extra_conf=0
let g:ycm_complete_in_comments=1
let g:ycm_collect_identifiers_from_comments_and_strings=1
let g:ycm_collect_identifiers_from_tags_files=1
let g:ycm_seed_identifiers_with_syntax=1
let g:ycm_filetype_blacklist = {'tagbar' : 1,'qf' : 1,'notes' : 1,'unite' : 1,'vimwiki' : 1,}
let g:ycm_error_symbol = '>>'
let g:ycm_warning_symbol = '>*'
"let g:ycm_server_use_vim_stdout = 1 
"let g:ycm_server_log_level = 'debug' 
