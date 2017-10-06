//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

// "Universal" script methods for projectile damage handling.  You can easily
// override these support functions with an equivalent namespace method if your
// weapon needs a unique solution for applying damage.

function ProjectileData::onCollision(%data, %proj, %col, %fade, %pos, %normal)
{
   //echo("ProjectileData::onCollision("@%data.getName()@", "@%proj@", "@%col.getClassName()@", "@%fade@", "@%pos@", "@%normal@")");

   // Apply damage to the object all shape base objects
   if (%data.directDamage > 0)
   {
      if (%col.getType() & ($TypeMasks::ShapeBaseObjectType))
         %col.damage(%proj, %pos, %data.directDamage, %data.damageType);
   }
   
   AlertAIPlayers(%proj.position,10,15,"Fire",3,%proj.sourceObject);
}