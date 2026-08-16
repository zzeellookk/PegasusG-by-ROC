PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS source (
  id INTEGER PRIMARY KEY,
  module_id TEXT NOT NULL,
  source_type TEXT NOT NULL DEFAULT 'local',
  display_name TEXT NOT NULL DEFAULT '',
  root_path TEXT NOT NULL,
  scan_rule TEXT NOT NULL DEFAULT '',
  selected INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL DEFAULT 0,
  UNIQUE(module_id, source_type, root_path)
);

CREATE TABLE IF NOT EXISTS item (
  id INTEGER PRIMARY KEY,
  source_id INTEGER NOT NULL REFERENCES source(id) ON DELETE CASCADE,
  module_id TEXT NOT NULL,
  type TEXT NOT NULL,
  title TEXT NOT NULL,
  path TEXT NOT NULL,
  added_at INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL DEFAULT 0,
  meta_json TEXT NOT NULL DEFAULT '{}',
  UNIQUE(module_id, path)
);

CREATE TABLE IF NOT EXISTS artwork (
  item_id INTEGER NOT NULL REFERENCES item(id) ON DELETE CASCADE,
  kind TEXT NOT NULL,
  cache_path TEXT NOT NULL,
  updated_at INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY(item_id, kind)
);

CREATE TABLE IF NOT EXISTS progress (
  item_id INTEGER PRIMARY KEY REFERENCES item(id) ON DELETE CASCADE,
  position TEXT NOT NULL DEFAULT '',
  percent INTEGER NOT NULL DEFAULT -1,
  updated_at INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS collection (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE,
  created_at INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS collection_item (
  collection_id INTEGER NOT NULL REFERENCES collection(id) ON DELETE CASCADE,
  item_id INTEGER NOT NULL REFERENCES item(id) ON DELETE CASCADE,
  added_at INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY(collection_id, item_id)
);

CREATE TABLE IF NOT EXISTS tag (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS item_tag (
  item_id INTEGER NOT NULL REFERENCES item(id) ON DELETE CASCADE,
  tag_id INTEGER NOT NULL REFERENCES tag(id) ON DELETE CASCADE,
  PRIMARY KEY(item_id, tag_id)
);

CREATE INDEX IF NOT EXISTS idx_item_module_title ON item(module_id, title);
CREATE INDEX IF NOT EXISTS idx_item_module_updated ON item(module_id, updated_at DESC);
CREATE INDEX IF NOT EXISTS idx_progress_updated ON progress(updated_at DESC);
