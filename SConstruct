import sys, os

env = Environment()
conf = Configure(env)

env['CPPDEFINES'] = []

env['CPPFLAGS'] = [ '-g'  ]

#env['CPPFLAGS'] = [ '-Ofast'  ]

env['CPPFLAGS'].append ( '-I' + os.getcwd() + '/src' )
env['CPPFLAGS'].append ( '-I/usr/local/include/soci/' )
env['CPPFLAGS'].append ( '-I/usr/include/postgresql/' )
env['LIBPATH'] =  [os.getcwd() + '/build', os.getcwd() + '/build/main' ]

print os.getcwd() + '/src/'
env.SConscript ('src/SConscript', variant_dir='build/main', exports=['env', 'conf'])
env.SConscript ('test/SConscript', variant_dir='build/test', exports=['env', 'conf'])

