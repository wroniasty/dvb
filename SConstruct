import sys, os

env = Environment()
conf = Configure(env)

env['CPPDEFINES'] = []
env['CPPFLAGS'] = [ '-g'  ]

env['CPPFLAGS'].append ( '-I' + os.getcwd() + '/src' )
env['LIBPATH'] =  [os.getcwd() + '/build' ]

print os.getcwd() + '/src/'
env.SConscript ('src/SConscript', variant_dir='build', exports=['env', 'conf'])
env.SConscript ('test/SConscript', exports=['env', 'conf'])

