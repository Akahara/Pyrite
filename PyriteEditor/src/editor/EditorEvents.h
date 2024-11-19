#pragma once

#include "utils/Delegate.h"


namespace pye
{
namespace EditorEvents
{

	static Delegate<class EditorActor*> OnActorSelectedEvent;
	static Delegate<> OnActorAddedEvent;
	static Delegate<> OnActorRemovedEvent;
	static Delegate<class EditorActor*> OnActorPickedEvent;






}
}