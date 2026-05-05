@0xd3f305eca0221509;

struct TTSRequest {
	text @0 :Text;
	speaker @1 :Text;
	language @2 :Text;
	rate @3 :Float32;
}

struct TTSBoundary {
	text @0 :Text;
	offset @1 :UInt64;
	duration @2 :UInt64;
}

struct TTSResponse {
	status @0 :Text;
	message @1 :Text;
	audio @2 :Data;
	boundaries @3 :List(TTSBoundary);
	duration @4 :UInt64;
}
