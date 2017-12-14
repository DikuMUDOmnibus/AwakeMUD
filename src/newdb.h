// file: newdb.h
// author: Andrew Hynek
// contents: class databii for players (todo: mobs, rooms, etc)

// for players: rnum = table index, vnum = id#

#ifndef __newdb_h__
#define __newdb_h__

class File;

class DBIndex
{
public:
  typedef long vnum_t;
  typedef long rnum_t;

  virtual ~DBIndex()
  {}

  static bool IsValidR(rnum_t idx)
  {
    return (idx >= 0);
  }
  static bool IsValidV(vnum_t virt)
  {
    return (virt >= 0);
  }

  virtual const char *GetNameR(const rnum_t) = 0;
  virtual const char *GetNameV(const vnum_t) = 0;

  virtual rnum_t GetRNUM(const char *) = 0;
  virtual rnum_t GetRNUM(const vnum_t) = 0;

  virtual vnum_t GetVNUM(const char *) = 0;
};

/* TODO
class ItemIndex : public DBIndex {
};
 
class NPCIndex : public DBIndex {
};
 
class QuestIndex : public DBIndex {
};
 
class RoomIndex : public DBIndex {
};
 
class ShopIndex : public DBIndex {
};
 
class VehicleIndex : public DBIndex {
};
*/

class PCIndex : public DBIndex
{
private:
  struct entry
  {
    char name[MAX_NAME_LENGTH];
    vnum_t id;
    int level;
    dword flags;
    time_t last;
    char_data *active_data;
    int instance_cnt;

    entry &operator=(const entry &two)
    {
      strcpy(name, two.name);
      id = two.id;
      level = two.level;
      flags = two.flags;
      last = two.last;
      active_data = two.active_data;
      instance_cnt = two.instance_cnt;

      return *this;
    }
  }
  *tab;

  // TODO: a tree structure

  int entry_cnt;
  int entry_size;
  bool needs_save;

  enum {
    NODELETE = (1 << 12)
  };

public:
  PCIndex();
  ~PCIndex();

  bool Load()
  {
    return load();
  }
  bool Save();

  // lookup/access functions
  //
  int NumPlayers()
  {
    return entry_cnt;
  }
  virtual const char *GetNameR(const rnum_t idx)
  {
    return get_name_by_idx(idx);
  }
  virtual const char *GetNameV(const vnum_t virt)
  {
    return get_name_by_idx(GetIndex(virt));
  }

  virtual rnum_t GetRNUM(const char *name)
  {
    return get_idx_by_name(name);
  }
  virtual rnum_t GetRNUM(const vnum_t virt)
  {
    return get_idx_by_id(virt);
  }

  rnum_t GetIndex(const char *name)
  {
    return GetRNUM(name);
  }
  rnum_t GetIndex(const vnum_t virt)
  {
    return GetRNUM(virt);
  }

  virtual vnum_t GetVNUM(const char *name)
  {
    return get_id_by_idx(GetIndex(name));
  }
  vnum_t GetID(const char *name)
  {
    return GetVNUM(name);
  }

  bool DoesExist(const vnum_t virt)
  {
    return (DBIndex::IsValidR(get_idx_by_id(virt)));
  }
  bool DoesExist(const char *name)
  {
    return (DBIndex::IsValidR(get_idx_by_name(name)));
  }

  // these functions return a pointer to the character if logged on, else NULL
  char_data *GetChar(const vnum_t virt)
  {
    return get_char_by_idx(GetIndex(virt));
  }
  char_data *GetChar(const char *name)
  {
    return get_char_by_idx(GetIndex(name));
  }

  // mutation functions
  //
  char_data *CreateChar(char_data *ch);

  bool DeleteChar(const vnum_t id)
  {
    return delete_by_idx(GetIndex(id));
  }
  bool DeleteChar(const char *name)
  {
    return delete_by_idx(GetIndex(name));
  }

  // loads the character and syncs IDs, returns the main struct
  // if logon is true, then the last logon time is set.
  char_data *LoadChar(const char *name, bool logon);

  // just saves the character
  bool SaveChar(char_data *ch, vnum_t loadroom = NOWHERE);

  // saves and puts character into storage
  // WARNING: this makes ch INVALID! (cause it returns the memory)
  bool StoreChar(char_data *ch, bool save = true);

  // updates the index
  void Update(char_data *ch);
  const char *get_name_by_idx(const rnum_t idx);
  char_data *get_char_by_idx(const rnum_t idx);
  vnum_t get_id_by_idx(const rnum_t idx);

private:
  void reset();
  bool load();
  int  count_entries(File *index);
  void sort_by_id();
  void resize_table(int empty_slots = 100);
  void sorted_insert(const entry *info);

  vnum_t find_open_id();

  rnum_t get_idx_by_name(const char *name);
  rnum_t get_idx_by_id(const vnum_t virt);

  bool delete_by_idx(rnum_t idx);
  void update_by_idx(rnum_t idx, char_data *ch);
  void clear_by_time();
  friend int entry_compare(const void *one, const void *two);
};

extern PCIndex playerDB;

#endif // ifndef __newdb_h__
