# My vimrc

Below my configuration for vim which I use for go and python development:
~~~
cat <<'EOF' > ~/.vimrc
call plug#begin()
  Plug 'vim-airline/vim-airline'
  Plug 'govim/govim', { 'for': 'go' }
  Plug 'preservim/nerdtree'
  Plug 'jistr/vim-nerdtree-tabs'
  Plug 'tpope/vim-fugitive'
  Plug 'jiangmiao/auto-pairs'
  if has('nvim')
    Plug 'Shougo/deoplete.nvim', { 'do': ':UpdateRemotePlugins' }
  else
    Plug 'Shougo/deoplete.nvim'
    Plug 'roxma/nvim-yarp'
    Plug 'roxma/vim-hug-neovim-rpc'
  endif
  Plug 'jnurmine/Zenburn'
  Plug 'vim-syntastic/syntastic'
  Plug 'nvie/vim-flake8'
  Plug 'davidhalter/jedi-vim'
  Plug 'ycm-core/YouCompleteMe'
call plug#end()
" For YouCompleteMe, see https://github.com/ycm-core/YouCompleteMe/issues/1751
" https://github.com/ycm-core/YouCompleteMe/tree/master#linux-64-bit
" PlugInstall
" cd ~/.vim/plugged/YouCompleteMe
" python3 install.py --clangd-completer

function! Toggles()
  :nnoremap <C-g> :NERDTreeTabsToggle<CR>
  " switch to tab right / left
  :nnoremap <C-l> :tabn<CR>
  :nnoremap <C-h> :tabp<CR>
  :nnoremap <C-t> :w!<CR>:!aspell check %<CR>:e! %<CR>
endfunction

function! GenericSetup()
  set tabstop=4
  set shiftwidth=4
  set expandtab
  " The vim-go autocomplete popup
  " will show up below and with only a single line
  set splitbelow
  set previewheight=1
  " Set VIM's working directory always to the current open file.
  " autocmd BufEnter * lcd %:p:h
  set listchars=tab:▸\ ,eol:¬
  " set invlist
  set number
  set encoding=utf-8

  filetype plugin on
  " set autochdir
  " color scheme
  " colorscheme zenburn
  colorscheme morning

  set colorcolumn=120
endfunction

function! DeopleteSetup()
  let g:deoplete#enable_at_startup = 1
  let g:deoplete#complete_method = "omnifunc"

  call deoplete#custom#option('omni_patterns', {
\ 'go': '[^. *\t]\.\w*',
\})
endfunction

function! AirlineSetup()
  " Set full path for vim-airline
  let g:airline_section_c = '%F'
endfunction

function! ConfigureYCM()
  let g:ycm_filetype_whitelist = { 'c': 1, 'cpp': 1 }
endfunction

call GenericSetup()
call Toggles()
call AirlineSetup()
call ConfigureYCM()
call DeopleteSetup()
EOF
~~~

~~~
cat <<'EOF' > ~/.vim/ftplugin/go.vim
function! GoVimSetup()
  autocmd BufNewFile,BufRead *.go setf go
  set nocompatible
  set nobackup
  set nowritebackup
  set noswapfile
  filetype plugin on
  " set mouse=a
  " set ttymouse=sgr
  set updatetime=500
  set balloondelay=250
  set signcolumn=yes
  autocmd! BufEnter,BufNewFile *.go,go.mod syntax on
  autocmd! BufLeave *.go,go.mod syntax off
  autocmd! BufWritePost *go !golangci-lint run --no-config --fast %
  set autoindent
  set smartindent
  filetype indent on
  set backspace=2
  if has("patch-8.1.1904")
    set completeopt+=popup
    set completepopup=align:menu,border:off,highlight:Pmenu
  endif
  :nnoremap <C-j> :call GOVIMHover()<CR>
  :nnoremap <C-k> :GOVIMReferences<CR>
  :inoremap <C-f> <C-o>:GOVIMFillStruct<CR>
  :inoremap <C-j> <C-o>:GOVIMExperimentalSignatureHelp<CR>

  set colorcolumn=120
  " let $GOVIM_GOPLS_FLAGS="-remote=auto; -remote.listen.timeout=12h"

  " mappings:
  " gd: godef
  " <CTRL>-T - go back
  " \h: hover information
  " \f: Autofill structs
  " \r: references
endfunction

call GoVimSetup()
EOF
~~~

~~~
cat <<'EOF' > ~/.vim/ftplugin/python.vim
function! SetupPython()
  " Enable syntax highlighting
  syntax on
  
  " Set tab width to 4 spaces
  set tabstop=4
  set softtabstop=4
  set shiftwidth=4
  set expandtab
  
  " Enable line numbers
  set number
  
  " Enable filetype detection
  filetype plugin on
  
  " Enable automatic indentation
  set autoindent
  
  " Enable file type-specific indentation
  set smartindent
  
  " Enable Syntastic for code linting
  " set statusline+=%#warningmsg#
  " set statusline+=%{SyntasticStatuslineFlag()}
  " set statusline+=%*
  " let g:syntastic_always_populate_loc_list = 1
  " let g:syntastic_auto_loc_list = 1
  " let g:syntastic_check_on_open = 1
  " let g:syntastic_check_on_wq = 0
 
  " Enable Ale for code linting
  let g:airline#extensions#ale#enabled = 1
  let g:ale_completion_enabled = 1
  let b:ale_fixers = ['add_blank_lines_for_python_control_statements', 'eslint', 'autoflake', 'autoimport', 'autopep8', 'black', 'isort', 'pycln', 'pyflyby', 'remove_trailing_lines', 'trim_whitespace']


  " Enable Jedi for code completion
  let g:jedi#auto_initialization = 1
  let g:jedi#popup_select_first = 1
  let g:jedi#popup_on_dot = 1
  
  " Set the Flake8 plugin as the Syntastic Python checker
  let g:syntastic_python_checkers = ['flake8']

  set colorcolumn=80

  let g:jedi#goto_assignments_command = "<C-k>"
  let g:jedi#goto_command  = "gd"
  noremap <C-t> <C-o>
endfunction

call SetupPython()
EOF
~~~

~~~
cat <<'EOF' > ~/.vim/ftplugin/c.vim
function! SetupC()
  " Enable syntax highlighting
  syntax on
  
  " Enable line numbers
  set number
  
  " Enable filetype detection
  filetype plugin on
  
  " Enable automatic indentation
  set autoindent
  
  " Enable file type-specific indentation
  set smartindent
  
  set colorcolumn=80

  :nnoremap gd :YcmCompleter GoTo<return>
endfunction

call SetupC()
EOF
~~~
