{
	testing:{
		tests:: "OpenSslTests.Main",
		file: "$(JDE_BUILD_DIR)/tests/test.txt"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		spd:{
			tags: {
				trace:["test", "io", "exception", "app"],
				debug:["settings"],
				information:[],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: "$(JDE_BUILD_DIR)", md: false }
			}
		}
	},
	workers:{
		executor: {threads: 2},
		io: {chunkByteSize: 10, threads: 2}
	}
}