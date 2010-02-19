function summon_alp(r, mage, level, force, params)
  local alp = unit.create(mage.faction, r, 1, "alp")
  local target = params[1]
  alp:set_skill("stealth", 7)
  alp.status = 5 -- FLEE
  a = attrib.create(alp, { type='alp', target=target })


  {
    /* Wenn der Alp stirbt, den Magier nachrichtigen */
    add_trigger(&alp->attribs, "destroy", trigger_unitmessage(mage, 
      "trigger_alp_destroy", MSG_EVENT, ML_INFO));
    /* Wenn Opfer oder Magier nicht mehr existieren, dann stirbt der Alp */
    add_trigger(&mage->attribs, "destroy", trigger_killunit(alp));
    add_trigger(&opfer->attribs, "destroy", trigger_killunit(alp));
  }
  msg = msg_message("summon_alp_effect", "mage alp target", mage, alp, opfer);
  r_addmessage(r, mage->faction, msg);
  msg_release(msg);
  
end
