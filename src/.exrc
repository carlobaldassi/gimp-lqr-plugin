if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
imap <M-S-Up> g<Up>i
imap <M-S-Down> g<Down>i
imap <C-S-Right> :call MoveTabRight()
imap <C-S-Left> :call MoveTabLeft()
imap <S-Right> :tabnext
imap <S-Left> :tabprevious
map! <S-Insert> <MiddleMouse>
map  :tabnew
map  <C-S-Left><S-Right>
map + ]z
map - [z
imap ¾ g$i
imap ¼ g0i
map _ :noh
nmap gx <Plug>NetrwBrowseX
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#NetrwBrowseX(expand("<cWORD>"),0)
nmap <S-F2> :call SaveSessionAndQuit()
nmap <F2> :call SaveSession()
map <M-S-Up> g<Up>
map <M-S-Down> g<Down>
map <C-S-Right> :call MoveTabRight()
map <C-S-Left> :call MoveTabLeft()
map <S-Right> gt
map <S-Left> gT
map <S-Insert> <MiddleMouse>
imap  :tabnew
map ­ zk
map « zj
map ¾ g$
map ¼ g0
map é i r 
let &cpo=s:cpo_save
unlet s:cpo_save
set background=dark
set backspace=2
set cindent
set confirm
set diffopt=filler,vertical
set expandtab
set fileencodings=ucs-bom,utf-8,default,latin1
set fileformats=unix,dos,mac
set fillchars=vert:\ ,fold:-,stl:\ ,stlnc:\ 
set formatoptions=croql
set formatlistpat=^\\s*\\(\\d\\|\\*\\|+\\)\\+[\\]:.)}\\t\ ]\\s*
set helplang=en
set history=1000
set hlsearch
set incsearch
set laststatus=2
set listchars=tab:|\ ,trail:.,extends:>,precedes:<,eol:$
set mouse=a
set printoptions=paper:a4
set report=0
set ruler
set runtimepath=~/.vim,/var/lib/vim/addons,/usr/share/vim/vimfiles,/usr/share/vim/vim71,/usr/share/vim/vimfiles/after,/var/lib/vim/addons/after,~/.vim/after
set scrolloff=5
set shiftwidth=2
set shortmess=aoOtTI
set showmatch
set smartindent
set statusline=%<%F\ %m%r%h%w\ [%{&ff}]\ %y%=%-14.(%l,%c%V%)\ [%L]\ %P
set suffixes=.bak,~,.swp,.o,.info,.aux,.log,.dvi,.bbl,.blg,.brf,.cb,.ind,.idx,.ilg,.inx,.out,.toc
set termencoding=utf-8
set viminfo='20,<50,s10,h,!
set whichwrap=b,s,<,>,h,l
set wildmenu
" vim: set ft=vim :
