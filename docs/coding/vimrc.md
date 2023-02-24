# My vimrc

Below my configuration for vim which I use for my daily go development:
~~~
autocmd BufNewFile,BufRead *.go setf go
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
call plug#end()


function! Toggles()
  :nnoremap <C-g> :NERDTreeTabsToggle<CR>
  " switch to tab right / left
  :nnoremap <C-l> :tabn<CR>
  :nnoremap <C-h> :tabp<CR>
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
  " set autochdir
  " color scheme
  colorscheme zenburn
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

function! GoVimSetup()
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
  " let $GOVIM_GOPLS_FLAGS="-remote=auto; -remote.listen.timeout=12h"

  " mappings:
  " gd: godef
  " <CTRL>-T - go back
  " \h: hover information
  " \f: Autofill structs
  " \r: references
endfunction

call GenericSetup()
call Toggles()
call GoVimSetup()
call DeopleteSetup()
call AirlineSetup()
~~~
