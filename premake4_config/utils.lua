--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local type = type

function isnil(x)      return type(x) == 'nil'      end
function isstring(x)   return type(x) == 'string'   end
function isboolean(x)  return type(x) == 'boolean'  end
function isnumber(x)   return type(x) == 'number'   end
function isuserdata(x) return type(x) == 'userdata' end
function istable(x)    return type(x) == 'table'    end
function isthread(x)   return type(x) == 'thread'   end
function isfunction(x) return type(x) == 'function' end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function table.makemap(t)
  local s = {}
  for i, v in ipairs(t) do
    s[v] = i
  end
  return s
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function io.readfile(name, mode)
  mode = mode or 'rt'
  local file, err = io.open(name, mode)
  if (not file) then
    return nil, err
  end
  local data, err = file:read('*a')
  file:close()
  if (not data) then
    return nil, err
  end
  return data
end


function io.writefile(name, data, mode)
  data = data or ''
  mode = mode or 'wt'
  local file, err = io.open(name, mode)
  if (not file) then
    return nil, err
  end
  if (not file:write(data)) then
    file:close()
    return nil, name .. ': write error'
  end
  file:close()
  return true
end


function stdoutf(fmt, ...)
  io.stdout:write(fmt:format(...))
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function os.outputof(cmd)
  local p, err = io.popen(cmd, 'r')
  if (not p) then
    return nil, err
  end
  local t = p:read('*a')
  p:close()
  return t
end

--------------------------------------------------------------------------------

function os.testcode(t)
  if (isstring(t)) then
    t = { code = t }
  end

  assert(isstring(t.code), 'missing "code" parameter')

  local source = ''

  local includes = t.includes or {}
  if (istable(includes)) then
    local incs = ''
    for _, inc in ipairs(includes) do
      incs = incs .. '#include '..inc..'\n'
    end
    includes = incs
  else
    includes = '#include ' .. includes .. '\n'
  end

  local incdirs = t.includedirs or {}
  local libdirs = t.libdirs     or {}
  local libs    = t.libs        or {}
  if (isstring(incdirs)) then incdirs = { incdirs } end
  if (isstring(libdirs)) then libdirs = { libdirs } end
  if (isstring(libs))    then libs    = { libs }    end

  assert(isstring(includes), 'invalid "includes" parameter')

  local code = t.code
  code = code:match('\n$') and code or (code .. '\n')

  source = source .. includes
  source = source .. 'int main(int argc, char** argv) {\n'
  source = source .. '  (void)argc; (void)argv;\n'
  source = source .. code
  source = source .. '}\n'

  --FIXME  print(source)

  local tmpCpp = os.tmpname()
  local tmpOut = os.tmpname()

  local f, err = io.open(tmpCpp, 'w')
  if (not f) then
    print(err)
    return false
  end
  f:write(source)
  f:close()

  local cmd = premake.gcc.cxx
  cmd = cmd .. ' ' .. '-o ' .. tmpOut
  cmd = cmd .. ' ' .. '-x c++'
  cmd = cmd .. ' ' .. tmpCpp
  for _, dir in ipairs(incdirs) do cmd = cmd .. ' ' .. '-I' .. dir end
  for _, dir in ipairs(libdirs) do cmd = cmd .. ' ' .. '-L' .. dir end
  for _, lib in ipairs(libs)    do cmd = cmd .. ' ' .. '-l' .. lib end
  cmd = cmd .. ' ' .. (t.buildoptions or '')

  if (1 > 0) then -- FIXME
    if (not os.is('windows')) then
      cmd = cmd .. ' &> /dev/null'
    else
      cmd = cmd .. ' > NUL 2>&1' -- FIXME - OS/2 & NT?
    end
  end

  local result = os.execute(cmd)

  os.remove(tmpOut)
  os.remove(tmpCpp)

  if (false and result ~= 0) then
    print(result)
    print(source)
    print(cmd)
  end
  return (result == 0)
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------