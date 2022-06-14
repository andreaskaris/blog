# My vimrc

~~~
call plug#begin()
Plug 'vim-airline/vim-airline'
Plug 'fatih/vim-go'
Plug 'preservim/nerdtree'
Plug 'jistr/vim-nerdtree-tabs'
Plug 'tpope/vim-fugitive'
if has('nvim')
  Plug 'Shougo/deoplete.nvim', { 'do': ':UpdateRemotePlugins' }
else
  Plug 'Shougo/deoplete.nvim'
  Plug 'roxma/nvim-yarp'
  Plug 'roxma/vim-hug-neovim-rpc'
endif
call plug#end()

function! Toggles()
  :nnoremap <C-g> :NERDTreeTabsToggle<CR>
  :nnoremap <C-l> :tabn<CR>
  :nnoremap <C-h> :tabp<CR>
endfunction

function! GenericSetup()
  set background=light
  set tabstop=4
  set shiftwidth=4
  set expandtab
  set termwinsize=4x0
  " The vim-go autocomplete popup
  " will show up below and with only a single line
  set splitbelow
  set previewheight=1
  " Set VIM's working directory always to the current open file.
  autocmd BufEnter * lcd %:p:h
  set listchars=tab:▸\ ,eol:¬
  " set invlist
  set number
  set encoding=utf-8
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

function! VimGoSetup()
  " vim-go related mappings
  au FileType go nmap <Leader>gi <Plug>(go-info)
  au FileType go nmap <Leader>gI <Plug>(go-install)
  au FileType go nmap <Leader>gs <Plug>(go-implements)
  au FileType go nmap <Leader>gr <Plug>(go-run)
  au FileType go nmap <Leader>gb <Plug>(go-build)
  au FileType go nmap <Leader>gt <Plug>(go-test)
  au FileType go nmap <Leader>gc <Plug>(go-coverage)
  au FileType go nmap <Leader>ge <Plug>(go-rename)
  au FileType go nmap <Leader>gd <Plug>(go-doc)
  au FileType go nmap <Leader>gv <Plug>(go-doc-vertical)
  au FileType go nmap <Leader>gB <Plug>(go-doc-browser)
  au FileType go nmap <Leader>gd <Plug>(go-def-vertical)
  au FileType go nmap <Leader>ds <Plug>(go-def-split)
  au FileType go nmap <Leader>dv <Plug>(go-def-vertical)
  au FileType go nmap <Leader>dt <Plug>(go-def-tab)
  let g:go_highlight_functions = 1
  let g:go_highlight_methods = 1
  let g:go_highlight_structs = 1
  let g:go_highlight_interfaces = 1
  let g:go_highlight_operators = 1
  let g:go_highlight_build_constraints = 1
endfunction

call GenericSetup()
call Toggles()
call VimGoSetup()
call DeopleteSetup()
call AirlineSetup()
~~~
