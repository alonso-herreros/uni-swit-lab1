#!/bin/bash

echo "🔄 Obteniendo ramas del remoto..."
git fetch --all --prune

echo "🔍 Recorriendo ramas remotas de origin..."
for remote_branch in $(git branch -r | grep -v '\->'); do
    local_branch=${remote_branch#origin/}
    echo "📦 Procesando rama: $local_branch"

    if git show-ref --verify --quiet refs/heads/$local_branch; then
        echo "🔄 Ya existe la rama local '$local_branch'. Haciendo merge (ff-only)..."
        git checkout $local_branch
        git merge --ff-only $remote_branch
    else
        echo "🆕 Rama local '$local_branch' no existe. Creando y siguiendo '$remote_branch'..."
        git checkout -b $local_branch --track $remote_branch
    fi
done

echo "✅ Todas las ramas sincronizadas."